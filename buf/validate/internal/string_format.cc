// Copyright 2023 Buf Technologies, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "buf/validate/internal/string_format.h"

#include "absl/strings/escaping.h"
#include "absl/strings/str_format.h"

namespace buf::validate::internal {
namespace cel = ::google::api::expr::runtime;

cel::CelValue StringFormat::format(
    google::protobuf::Arena* arena,
    cel::CelValue::StringHolder formatVal,
    cel::CelValue arg) const {
  if (!arg.IsList()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, "format: expected list");
    return cel::CelValue::CreateError(error);
  }
  const cel::CelList& list = *arg.ListOrDie();
  std::string_view fmtStr = formatVal.value();
  auto* result = google::protobuf::Arena::Create<std::string>(arena);
  auto status = format(*result, fmtStr, list);
  if (!status.ok()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, status.message());
    return cel::CelValue::CreateError(error);
  }
  return cel::CelValue::CreateString(result);
}

absl::Status StringFormat::format(
    std::string& builder, std::string_view fmtStr, const cel::CelList& args) const {
  size_t index = 0;
  size_t argIndex = 0;
  while (index < fmtStr.size()) {
    char c = fmtStr[index++];
    if (c != '%') {
      builder.push_back(c);
      // Add the entire character if it's not a UTF-8 character.
      if ((c & 0x80) != 0) {
        // Add the rest of the UTF-8 character.
        while (index < fmtStr.size() && (fmtStr[index] & 0xc0) == 0x80) {
          builder.push_back(fmtStr[index++]);
        }
      }
      continue;
    }
    if (index >= fmtStr.size()) {
      return absl::InvalidArgumentError("format: expected format specifier");
    }
    if (fmtStr[index] == '%') {
      builder.push_back('%');
      index++;
      continue;
    }
    if (argIndex >= static_cast<int>(args.size())) {
      return absl::InvalidArgumentError("format: not enough arguments");
    }
    const cel::CelValue& arg = args[argIndex++];
    c = fmtStr[index++];
    int precision = 6;
    if (c == '.') {
      // parse the precision
      precision = 0;
      while (index < fmtStr.size() && '0' <= fmtStr[index] && fmtStr[index] <= '9') {
        precision = precision * 10 + (fmtStr[index++] - '0');
      }
      if (index >= fmtStr.size()) {
        return absl::InvalidArgumentError("format: expected format specifier");
      }
      c = fmtStr[index++];
    }

    absl::Status status;
    switch (c) {
      case 'e':
        status = formatExponent(builder, arg, precision);
        break;
      case 'f':
        status = formatFloating(builder, arg, precision);
        break;
      case 'b':
        status = formatBinary(builder, arg);
        break;
      case 'o':
        status = formatOctal(builder, arg);
        break;
      case 'd':
        status = formatDecimal(builder, arg);
        break;
      case 'x':
        status = formatHex(builder, arg);
        break;
      case 'X':
        status = formatHeX(builder, arg);
        break;
      case 's':
        status = formatString(builder, arg);
        break;
      default:
        return absl::InvalidArgumentError("format: invalid format specifier");
    }
    if (!status.ok()) {
      return status;
    }
  }
  if (argIndex < static_cast<int>(args.size())) {
    return absl::InvalidArgumentError("format: too many arguments");
  }
  return absl::OkStatus();
}

absl::Status StringFormat::formatExponent(
    std::string& builder, const google::api::expr::runtime::CelValue& value, int precision) const {
  if (!value.IsDouble()) {
    return absl::InvalidArgumentError("formatExponent: expected double");
  }
  builder += absl::StrFormat("%.*e", precision, value.DoubleOrDie());
  return absl::OkStatus();
}
absl::Status StringFormat::formatFloating(
    std::string& builder, const google::api::expr::runtime::CelValue& value, int precision) const {
  if (!value.IsDouble()) {
    return absl::InvalidArgumentError("formatFloating: expected double");
  }
  builder += absl::StrFormat("%.*f", precision, value.DoubleOrDie());
  return absl::OkStatus();
}

void StringFormat::formatUnsigned(
    std::string& builder, uint64_t value, int base, const char* digits) const {
  if (value == 0) {
    builder += "0";
    return;
  }
  char buf[64];
  int index = sizeof(buf);
  while (value != 0 && index > 1) {
    buf[--index] = digits[value % base];
    value /= base;
  }
  builder.append(buf + index, sizeof(buf) - index);
}

void StringFormat::formatInteger(
    std::string& builder, int64_t value, int base, const char* digits) const {
  if (value < 0) {
    builder += "-";
    value = -value;
  }
  formatUnsigned(builder, value, base, digits);
}

absl::Status StringFormat::formatBinary(
    std::string& builder, const google::api::expr::runtime::CelValue& value) const {
  switch (value.type()) {
    case cel::CelValue::Type::kBool:
      builder += value.BoolOrDie() ? "1" : "0";
      return absl::OkStatus();
    case cel::CelValue::Type::kInt64:
      formatInteger(builder, value.Int64OrDie(), 2);
      return absl::OkStatus();
    case cel::CelValue::Type::kUint64:
      formatUnsigned(builder, value.Uint64OrDie(), 2);
      return absl::OkStatus();
    default:
      return absl::InvalidArgumentError("formatBinary: expected integer");
  }
}

absl::Status StringFormat::formatOctal(
    std::string& builder, const google::api::expr::runtime::CelValue& value) const {
  switch (value.type()) {
    case cel::CelValue::Type::kInt64:
      formatInteger(builder, value.Int64OrDie(), 8);
      return absl::OkStatus();
    case cel::CelValue::Type::kUint64:
      formatUnsigned(builder, value.Uint64OrDie(), 8);
      return absl::OkStatus();
    default:
      return absl::InvalidArgumentError("formatOctal: expected integer");
  }
}

absl::Status StringFormat::formatDecimal(
    std::string& builder, const google::api::expr::runtime::CelValue& value) const {
  switch (value.type()) {
    case cel::CelValue::Type::kInt64:
      formatInteger(builder, value.Int64OrDie(), 10);
      return absl::OkStatus();
    case cel::CelValue::Type::kUint64:
      formatUnsigned(builder, value.Uint64OrDie(), 10);
      return absl::OkStatus();
    default:
      return absl::InvalidArgumentError("formatDecimal: expected integer");
  }
}

void StringFormat::formatHexString(std::string& builder, std::string_view value) const {
  for (const char c : value) {
    formatUnsigned(builder, static_cast<uint8_t>(c), 16, kLowerHexDigits);
  }
}

void StringFormat::formatHeXString(std::string& builder, std::string_view value) const {
  for (const char c : value) {
    formatUnsigned(builder, static_cast<uint8_t>(c), 16, kUpperHexDigits);
  }
}

absl::Status StringFormat::formatHex(
    std::string& builder, const google::api::expr::runtime::CelValue& value) const {
  switch (value.type()) {
    case cel::CelValue::Type::kInt64:
      formatInteger(builder, value.Int64OrDie(), 16, kLowerHexDigits);
      break;
    case cel::CelValue::Type::kUint64:
      formatUnsigned(builder, value.Uint64OrDie(), 16, kLowerHexDigits);
      break;
    case cel::CelValue::Type::kBytes:
      formatHexString(builder, value.BytesOrDie().value());
      break;
    case cel::CelValue::Type::kString:
      formatHexString(builder, value.StringOrDie().value());
      break;
    default:
      return absl::InvalidArgumentError("formatHex: expected integer or string");
  }
  return absl::OkStatus();
}

absl::Status StringFormat::formatHeX(
    std::string& builder, const google::api::expr::runtime::CelValue& value) const {
  switch (value.type()) {
    case cel::CelValue::Type::kInt64:
      formatInteger(builder, value.Int64OrDie(), 16, kUpperHexDigits);
      break;
    case cel::CelValue::Type::kUint64:
      formatUnsigned(builder, value.Uint64OrDie(), 16, kUpperHexDigits);
      break;
    case cel::CelValue::Type::kBytes:
      formatHeXString(builder, value.BytesOrDie().value());
      break;
    case cel::CelValue::Type::kString:
      formatHeXString(builder, value.StringOrDie().value());
      break;
    default:
      return absl::InvalidArgumentError("formatHex: expected integer or string");
  }
  return absl::OkStatus();
}

absl::Status StringFormat::formatString(
    std::string& builder, const google::api::expr::runtime::CelValue& val) const {
  switch (val.type()) {
    case cel::CelValue::Type::kString:
      absl::StrAppend(&builder, val.StringOrDie().value());
      break;
    case cel::CelValue::Type::kBytes:
      absl::StrAppend(&builder, val.BytesOrDie().value());
      break;
    default:
      return formatStringSafe(builder, val);
  }
  return absl::OkStatus();
}

absl::Status StringFormat::formatStringSafe(
    std::string& builder, const google::api::expr::runtime::CelValue& val) const {
  switch (val.type()) {
    default:
      break;
    case cel::CelValue::Type::kBool:
      builder += val.BoolOrDie() ? "true" : "false";
      break;
    case cel::CelValue::Type::kInt64:
      formatInteger(builder, val.Int64OrDie(), 10);
      break;
    case cel::CelValue::Type::kUint64:
      formatUnsigned(builder, val.Uint64OrDie(), 10);
      break;
    case cel::CelValue::Type::kDouble:
      builder += absl::StrCat(val.DoubleOrDie());
      break;
    case cel::CelValue::Type::kString:
      absl::StrAppend(&builder, "\"", absl::CEscape(val.StringOrDie().value()), "\"");
      break;
    case cel::CelValue::Type::kBytes:
      absl::StrAppend(&builder, "b\"", absl::CEscape(val.BytesOrDie().value()), "\"");
      break;
    case cel::CelValue::Type::kDuration:
      builder += "duration('";
      builder += absl::FormatDuration(val.DurationOrDie());
      builder += "')";
      break;
    case cel::CelValue::Type::kTimestamp:
      builder += "timestamp('";
      builder += absl::FormatTime(val.TimestampOrDie());
      builder += "')";
      break;
    case cel::CelValue::Type::kList: {
      builder += "[";
      const char* delim = "";
      const auto& list = *val.ListOrDie();
      for (int i = 0; i < list.size(); i++) {
        if (auto status = formatStringSafe(builder, list[i]); !status.ok()) {
          return status;
        }
        builder += delim;
        delim = ", ";
      }
      builder += "]";
      break;
    }
    case cel::CelValue::Type::kMap: {
      builder += "{";
      const char* delim = "";
      const auto& map = *val.MapOrDie();
      auto keys_or = map.ListKeys();
      if (!keys_or.ok()) {
        return keys_or.status();
      }
      const auto& keys = *keys_or.value();
      for (int i = 0; i < keys.size(); i++) {
        builder += delim;
        if (auto status = formatStringSafe(builder, keys[i]); !status.ok()) {
          return status;
        }
        builder += ": ";
        auto mapVal = map[keys[i]];
        if (mapVal.has_value()) {
          if (auto status = formatStringSafe(builder, mapVal.value()); !status.ok()) {
            return status;
          }
        }
        delim = ", ";
      }
      break;
    }
    case cel::CelValue::Type::kMessage:
      builder += val.MessageOrDie()->DebugString();
      break;
    case cel::CelValue::Type::kNullType:
      builder += "null";
      break;
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal
