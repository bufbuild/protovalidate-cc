#include "buf/validate/internal/extra_func.h"

#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "fmt/core.h"
#include "google/protobuf/arena.h"

namespace buf::validate::internal {
namespace cel = google::api::expr::runtime;

cel::CelError formatExponent(std::string& builder, const cel::CelValue& val, int precision) {
  if (!val.IsDouble()) {
    return cel::CelError(absl::StatusCode::kInvalidArgument, "formatExponent: expected double");
  }
  builder += absl::StrFormat("%.*e", precision, val.DoubleOrDie());
  return absl::OkStatus();
}

cel::CelError formatFloating(std::string& builder, const cel::CelValue& val, int precision) {
  if (!val.IsDouble()) {
    return cel::CelError(absl::StatusCode::kInvalidArgument, "formatFloating: expected double");
  }
  builder += absl::StrFormat("%.*f", precision, val.DoubleOrDie());
  return absl::OkStatus();
}

const char kUpperHexDigits[17] = "0123456789ABCDEF";
const char kLowerHexDigits[17] = "0123456789abcdef";

void uintToString(
    std::string& builder, uint64_t value, int base, const char digits[17] = kUpperHexDigits) {
  if (value == 0) {
    builder += "0";
    return;
  }
  char buffer[64];
  int i = 0;
  while (value > 0) {
    buffer[i++] = digits[value % base];
    value /= base;
  }
  while (i > 0) {
    builder += buffer[--i];
  }
}

void intToString(
    std::string& builder, int64_t value, int base, const char digits[17] = kUpperHexDigits) {
  if (value < 0) {
    builder += "-";
    value = -value;
  }
  uintToString(builder, value, base, digits);
}

cel::CelError formatBinary(std::string& builder, const cel::CelValue& val) {
  switch (val.type()) {
    case cel::CelValue::Type::kBool:
      builder += val.BoolOrDie() ? "1" : "0";
      break;
    case cel::CelValue::Type::kInt64:
      intToString(builder, val.Int64OrDie(), 2);
      break;
    case cel::CelValue::Type::kUint64:
      uintToString(builder, val.Uint64OrDie(), 2);
      break;
    default:
      return cel::CelError(absl::StatusCode::kInvalidArgument, "formatBinary: expected int");
  }
  return absl::OkStatus();
}

cel::CelError formatOctal(std::string& builder, const cel::CelValue& val) {
  switch (val.type()) {
    case cel::CelValue::Type::kInt64:
      intToString(builder, val.Int64OrDie(), 8);
      break;
    case cel::CelValue::Type::kUint64:
      uintToString(builder, val.Uint64OrDie(), 8);
      break;
    default:
      return cel::CelError(absl::StatusCode::kInvalidArgument, "formatOctal: expected int");
  }
  return absl::OkStatus();
}

cel::CelError formatDecimal(std::string& builder, const cel::CelValue& val) {
  switch (val.type()) {
    case cel::CelValue::Type::kInt64:
      intToString(builder, val.Int64OrDie(), 10);
      break;
    case cel::CelValue::Type::kUint64:
      uintToString(builder, val.Uint64OrDie(), 10);
      break;
    default:
      return cel::CelError(absl::StatusCode::kInvalidArgument, "formatDecimal: expected int");
  }
  return absl::OkStatus();
}

cel::CelError formatHexString(std::string& builder, const absl::string_view& val) {
  for (const char c : val) {
    uintToString(builder, static_cast<uint8_t>(c), 16, kLowerHexDigits);
  }
  return absl::OkStatus();
}

cel::CelError formatHeXString(std::string& builder, const absl::string_view& val) {
  for (const char c : val) {
    uintToString(builder, static_cast<uint8_t>(c), 16, kUpperHexDigits);
  }
  return absl::OkStatus();
}

cel::CelError formatHex(std::string& builder, const cel::CelValue& val) {
  switch (val.type()) {
    case cel::CelValue::Type::kInt64:
      intToString(builder, val.Int64OrDie(), 16, kLowerHexDigits);
      break;
    case cel::CelValue::Type::kUint64:
      uintToString(builder, val.Uint64OrDie(), 16, kLowerHexDigits);
      break;
    case cel::CelValue::Type::kBytes:
      return formatHexString(builder, val.BytesOrDie().value());
    case cel::CelValue::Type::kString:
      return formatHeXString(builder, val.StringOrDie().value());
    default:
      return cel::CelError(absl::StatusCode::kInvalidArgument, "formatHex: expected int");
  }
  return absl::OkStatus();
}

cel::CelError formatHeX(std::string& builder, const cel::CelValue& val) {
  switch (val.type()) {
    case cel::CelValue::Type::kInt64:
      intToString(builder, val.Int64OrDie(), 16, kUpperHexDigits);
      break;
    case cel::CelValue::Type::kUint64:
      uintToString(builder, val.Uint64OrDie(), 16, kUpperHexDigits);
      break;
    case cel::CelValue::Type::kBytes:
      return formatHexString(builder, val.BytesOrDie().value());
    case cel::CelValue::Type::kString:
      return formatHeXString(builder, val.StringOrDie().value());
    default:
      return cel::CelError(absl::StatusCode::kInvalidArgument, "formatHeX: expected int");
  }
  return absl::OkStatus();
}

cel::CelError formatString(std::string& builder, const cel::CelValue& val) {
  switch (val.type()) {
    case cel::CelValue::Type::kBool:
      builder += val.BoolOrDie() ? "true" : "false";
      break;
    case cel::CelValue::Type::kInt64:
      intToString(builder, val.Int64OrDie(), 10);
      break;
    case cel::CelValue::Type::kUint64:
      uintToString(builder, val.Uint64OrDie(), 10);
      break;
    case cel::CelValue::Type::kDouble:
      builder += absl::StrCat(val.DoubleOrDie());
      break;
    case cel::CelValue::Type::kString:
      builder += val.StringOrDie().value();
      break;
    case cel::CelValue::Type::kBytes:
      builder += val.BytesOrDie().value();
      break;
    case cel::CelValue::Type::kDuration:
      builder += absl::FormatDuration(val.DurationOrDie());
      break;
    case cel::CelValue::Type::kTimestamp:
      builder += absl::FormatTime(val.TimestampOrDie());
      break;
    case cel::CelValue::Type::kList:
      //      builder += "[";
      //      for (const auto& v : *val.ListOrDie()) {
      //        formatString(builder, v);
      //        builder += ", ";
      //      }
      //      builder += "]";
      break;
    case cel::CelValue::Type::kMap:
      //      builder += "{";
      //      for (const auto& v : *val.MapOrDie()) {
      //        formatString(builder, v.first);
      //        builder += ": ";
      //        formatString(builder, v.second);
      //        builder += ", ";
      //      }
      //      builder += "}";
      break;
    case cel::CelValue::Type::kMessage:
      builder += val.MessageOrDie()->DebugString();
      break;
    case cel::CelValue::Type::kNullType:
      builder += "null";
      break;
  }
  return absl::OkStatus();
}

cel::CelValue format(
    google::protobuf::Arena* arena, cel::CelValue::StringHolder format, cel::CelValue arg) {
  auto result = google::protobuf::Arena::Create<std::string>(arena);
  std::string& builder = *result;
  if (!arg.IsList()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, "format: expected list");
    return cel::CelValue::CreateError(error);
  }
  const cel::CelList& list = *arg.ListOrDie();
  std::string_view fmtStr = format.value();
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
      auto error = google::protobuf::Arena::Create<cel::CelError>(
          arena, absl::StatusCode::kInvalidArgument, "format: expected format specifier");
      return cel::CelValue::CreateError(error);
    }
    if (fmtStr[index] == '%') {
      builder.push_back('%');
      index++;
      continue;
    }
    if (argIndex >= list.size()) {
      auto error = google::protobuf::Arena::Create<cel::CelError>(
          arena, absl::StatusCode::kInvalidArgument, "format: not enough arguments");
      return cel::CelValue::CreateError(error);
    }
    const cel::CelValue& arg = list[argIndex++];
    c = fmtStr[index++];
    int precision = 6;
    if (c == '.') {
      // parse the precision
      precision = 0;
      while (index < fmtStr.size() && '0' <= fmtStr[index] && fmtStr[index] <= '9') {
        precision = precision * 10 + (fmtStr[index++] - '0');
      }
      if (index >= fmtStr.size()) {
        auto error = google::protobuf::Arena::Create<cel::CelError>(
            arena, absl::StatusCode::kInvalidArgument, "format: expected format specifier");
        return cel::CelValue::CreateError(error);
      }
      c = fmtStr[index++];
    }

    cel::CelError status;
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
        auto error = google::protobuf::Arena::Create<cel::CelError>(
            arena, absl::StatusCode::kInvalidArgument, "format: unknown format specifier");
        return cel::CelValue::CreateError(error);
    }
    if (!status.ok()) {
      return cel::CelValue::CreateError(
          google::protobuf::Arena::Create<cel::CelError>(arena, status));
    }
  }
  return cel::CelValue::CreateString(result);
}

absl::Status RegisterExtraFuncs(google::api::expr::runtime::CelFunctionRegistry& registry) {
  auto status = cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder, cel::CelValue>::
      CreateAndRegister("format", true, &format, &registry);
  if (!status.ok()) {
    return status;
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal