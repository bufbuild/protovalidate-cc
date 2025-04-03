// Copyright 2023-2025 Buf Technologies, Inc.
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

#include "buf/validate/internal/extra_func.h"

#include <string>
#include <string_view>

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "buf/validate/internal/lib/ipv4.h"
#include "buf/validate/internal/lib/ipv6.h"
#include "buf/validate/internal/string_format.h"
#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "eval/public/containers/container_backed_map_impl.h"
#include "google/protobuf/arena.h"
#include "re2/re2.h"

namespace buf::validate::internal {

bool isPathValid(const std::string_view path) {
  if (path == "/") {
    return true;
  }
  std::string stringPath(path);
  /**
   * ^: Matches the start of the string.
   * [\/]*: Matches zero or more occurrences of the forward slash ("/") character.
   * [\w\/\-\.]*: Matches zero or more occurrences of the following characters:
   *    \w: Matches any alphanumeric character (A-Z, a-z, 0-9) or underscore (_).
   *    \/: Matches the forward slash ("/") character.
   *    \-: Matches the hyphen ("-") character.
   *    \.: Matches the period (dot, ".") character.
   * $: Matches the end of the string.
   */
  re2::RE2 pathPattern(R"(^[\/]*[\w\/\-\.]*$)");
  return re2::RE2::FullMatch(stringPath, pathPattern);
}

namespace cel = google::api::expr::runtime;

cel::CelValue isNan(google::protobuf::Arena* arena, cel::CelValue rhs) {
  if (!rhs.IsDouble()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, "expected a double value");
    return cel::CelValue::CreateError(error);
  }
  return cel::CelValue::CreateBool(std::isnan(rhs.DoubleOrDie()));
}

cel::CelValue isInfX(google::protobuf::Arena* arena, cel::CelValue rhs, cel::CelValue sign) {
  if (!rhs.IsDouble()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, "expected a double value");
    return cel::CelValue::CreateError(error);
  }
  if (!sign.IsInt64()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, "expected an int64 value");
    return cel::CelValue::CreateError(error);
  }
  double value = rhs.DoubleOrDie();
  int64_t sign_value = sign.Int64OrDie();
  if (sign_value > 0) {
    return cel::CelValue::CreateBool(std::isinf(value) && value > 0);
  } else if (sign_value < 0) {
    return cel::CelValue::CreateBool(std::isinf(value) && value < 0);
  } else {
    return cel::CelValue::CreateBool(std::isinf(value));
  }
}

cel::CelValue isInf(google::protobuf::Arena* arena, cel::CelValue rhs) {
  return isInfX(arena, rhs, cel::CelValue::CreateInt64(0));
}

cel::CelValue unique(google::protobuf::Arena* arena, cel::CelValue rhs) {
  if (!rhs.IsList()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, "expected a list value");
    return cel::CelValue::CreateError(error);
  }
  const cel::CelList& cel_list = *rhs.ListOrDie();
  cel::CelMapBuilder cel_map = *google::protobuf::Arena::Create<cel::CelMapBuilder>(arena);
  for (int index = 0; index < cel_list.size(); index++) {
    cel::CelValue cel_value = cel_list[index];
    auto status = cel_map.Add(cel_value, cel_value);
    if (!status.ok()) {
      return cel::CelValue::CreateBool(false);
    }
  }
  return cel::CelValue::CreateBool(cel_list.size() == cel_map.size());
}

cel::CelValue contains(
    google::protobuf::Arena* arena, cel::CelValue::BytesHolder lhs, cel::CelValue rhs) {
  if (!rhs.IsBytes()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, "does not contain the right value");
    return cel::CelValue::CreateError(error);
  }
  bool result = absl::StrContains(lhs.value().data(), rhs.BytesOrDie().value());
  return cel::CelValue::CreateBool(result);
}

cel::CelValue startsWith(
    google::protobuf::Arena* arena, cel::CelValue::BytesHolder lhs, cel::CelValue rhs) {
  if (!rhs.IsBytes()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, "doesnt start with the right thing");
    return cel::CelValue::CreateError(error);
  }
  bool result = absl::StartsWith(lhs.value().data(), rhs.BytesOrDie().value());
  return cel::CelValue::CreateBool(result);
}

cel::CelValue endsWith(
    google::protobuf::Arena* arena, cel::CelValue::BytesHolder lhs, cel::CelValue rhs) {
  if (!rhs.IsBytes()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, "doesnt end with the right thing");
    return cel::CelValue::CreateError(error);
  }
  bool result = absl::EndsWith(lhs.value().data(), rhs.BytesOrDie().value());
  return cel::CelValue::CreateBool(result);
}

bool IsHostname(std::string_view to_validate) {
  if (to_validate.length() > 253) {
    return false;
  }
  static const re2::RE2 component_regex("^[A-Za-z0-9]+(?:-[A-Za-z0-9]+)*$");
  static const re2::RE2 all_digits("^[0-9]*$");
  to_validate = absl::StripSuffix(to_validate, ".");
  std::vector<std::string_view> split = absl::StrSplit(to_validate, '.');
  if (split.size() < 2) {
    return re2::RE2::FullMatch(to_validate, component_regex) &&
        !re2::RE2::FullMatch(to_validate, all_digits);
  }
  if (re2::RE2::FullMatch(split[split.size() - 1], all_digits)) {
    return false;
  }
  for (size_t i = 0; i < split.size(); i++) {
    const std::string_view& part = split[i];
    if (part.empty() || part.size() > 63) {
      return false;
    }
    if (!re2::RE2::FullMatch(part, component_regex)) {
      return false;
    }
  }
  return true;
}

cel::CelValue isHostname(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  return cel::CelValue::CreateBool(IsHostname(lhs.value()));
}

cel::CelValue isEmail(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  // Based on https://html.spec.whatwg.org/multipage/input.html#valid-e-mail-address
  static const re2::RE2 regex(
      "^[a-zA-Z0-9.!#$%&'*+\\/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$");
  return cel::CelValue::CreateBool(re2::RE2::FullMatch(lhs.value(), regex));
}

bool IsIpv4(const std::string_view toValidate) {
  return lib::parseIPv4Address(toValidate).has_value();
}

bool IsIpv6(const std::string_view toValidate) {
  return lib::parseIPv6Address(toValidate).has_value();
}

bool IsIp(const std::string_view to_validate) {
  return IsIpv4(to_validate) || IsIpv6(to_validate);
}

cel::CelValue isIPvX(
    google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs, cel::CelValue rhs) {
  std::string_view str = lhs.value();
  switch (rhs.Int64OrDie()) {
    case 0:
      return cel::CelValue::CreateBool(IsIp(str));
    case 4:
      return cel::CelValue::CreateBool(IsIpv4(str));
    case 6:
      return cel::CelValue::CreateBool(IsIpv6(str));
    default:
      return cel::CelValue::CreateBool(false);
  }
}

cel::CelValue isIP(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  return isIPvX(arena, lhs, cel::CelValue::CreateInt64(0));
}

bool IsPort(const std::string_view str) {
  uint32_t port;
  if (!absl::SimpleAtoi(str, &port)) {
    return false;
  }
  static constexpr uint32_t MAX_PORT = 65535;
  return port <= MAX_PORT;
}

bool IsHostAndPort(const std::string_view str, const bool portRequired) {
  if (str.empty()) {
    return false;
  }

  const auto splitIdx = str.rfind(':');
  if (str.front() == '[') {
    const auto end = str.find(']');
    const auto afterEnd = end + 1;
    if (afterEnd == str.size()) { // no port
      const auto host = str.substr(1, end - 1);
      return !portRequired && IsIpv6(host);
    }
    if (afterEnd == splitIdx) { // port
      const auto host = str.substr(1, end - 1);
      const auto port = str.substr(splitIdx + 1);
      return IsIpv6(host) && IsPort(port);
    }
    return false;
  }

  if (splitIdx == std::string_view::npos) {
    return !portRequired && (IsHostname(str) || IsIpv4(str));
  }
  const auto host = str.substr(0, splitIdx);
  const auto port = str.substr(splitIdx + 1);
  return (IsHostname(host) || IsIpv4(host)) && IsPort(port);
}

cel::CelValue isHostAndPort(
    google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs, cel::CelValue rhs) {
  const std::string_view str = lhs.value();
  const bool portReq = rhs.BoolOrDie();
  return cel::CelValue::CreateBool(IsHostAndPort(str, portReq));
}

/**
 * IP Prefix Validation
 */
bool IsIpv4Prefix(const std::string_view toValidate, bool strict) {
  auto prefix = lib::parseIPv4Prefix(toValidate);
  if (!prefix) {
    return false;
  }
  if (!strict) {
    return true;
  }
  // Check that the host ID part is all zero when strict is true.
  return (prefix->bits & ~prefix->mask()) == 0;
}

bool IsIpv6Prefix(const std::string_view toValidate, bool strict) {
  auto prefix = lib::parseIPv6Prefix(toValidate);
  if (!prefix) {
    return false;
  }
  if (!strict) {
    return true;
  }
  // Check that the host ID part is all zero when strict is true.
  return (prefix->bits & ~prefix->mask()) == 0;
}

bool IsIpPrefix(const std::string_view toValidate, bool strict) {
  return IsIpv4Prefix(toValidate, strict) || IsIpv6Prefix(toValidate, strict);
}

cel::CelValue isIpPrefixXY(
    google::protobuf::Arena* arena,
    cel::CelValue::StringHolder lhs,
    cel::CelValue rhs1,
    cel::CelValue rhs2) {
  std::string_view str = lhs.value();
  int ver = rhs1.Int64OrDie();
  bool strict = rhs2.BoolOrDie();

  switch (ver) {
    case 0:
      return cel::CelValue::CreateBool(IsIpPrefix(str, strict));
    case 4:
      return cel::CelValue::CreateBool(IsIpv4Prefix(str, strict));
    case 6:
      return cel::CelValue::CreateBool(IsIpv6Prefix(str, strict));
    default:
      return cel::CelValue::CreateBool(false);
  }
}

cel::CelValue isIpPrefixX(
    google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs, cel::CelValue rhs) {
  if (rhs.IsBool()) {
    return isIpPrefixXY(arena, lhs, cel::CelValue::CreateInt64(0), rhs);
  }
  return isIpPrefixXY(arena, lhs, rhs, cel::CelValue::CreateBool(false));
}

cel::CelValue isIpPrefix(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  return isIpPrefixXY(arena, lhs, cel::CelValue::CreateInt64(0), cel::CelValue::CreateBool(false));
}

/**
 * Naive URI validation.
 */
cel::CelValue isUri(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  const std::string_view& ref = lhs.value();
  if (ref.empty()) {
    return cel::CelValue::CreateBool(false);
  }
  std::string_view scheme, host;
  if (!absl::StrContains(ref, "://")) {
    return cel::CelValue::CreateBool(false);
  }
  std::vector<std::string_view> split = absl::StrSplit(ref, absl::MaxSplits("://", 1));
  scheme = split[0];
  std::vector<std::string_view> hostSplit = absl::StrSplit(split[1], absl::MaxSplits('/', 1));
  host = hostSplit[0];
  // Just checking that scheme and host are present.
  return cel::CelValue::CreateBool(!scheme.empty() && !host.empty());
}

/**
 * Naive URI ref validation.
 */
cel::CelValue isUriRef(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  const std::string_view& ref = lhs.value();
  if (ref.empty()) {
    return cel::CelValue::CreateBool(false);
  }
  std::string_view scheme, host, path;
  std::string_view remainder = ref;
  if (absl::StrContains(ref, "://")) {
    std::vector<std::string_view> split = absl::StrSplit(ref, absl::MaxSplits("://", 1));
    scheme = split[0];
    std::vector<std::string_view> hostSplit = absl::StrSplit(split[1], absl::MaxSplits('/', 1));
    host = hostSplit[0];
    // If hostSplit has a size greater than 1, then a '/' appeared in the string. Set the rest
    // to remainder so we can parse any query string.
    if (hostSplit.size() > 1) {
      remainder = hostSplit[1];
    }
  }
  std::vector<std::string_view> querySplit = absl::StrSplit(remainder, absl::MaxSplits('?', 1));
  path = querySplit[0];
  if (!isPathValid(path)) {
    return cel::CelValue::CreateBool(false);
  }
  // If the scheme and host are invalid, then the input is a URI ref (so make sure path exists).
  // If the scheme and host are valid, then the input is a URI.
  bool parsedResult = !path.empty() || (!scheme.empty() && !host.empty());
  return cel::CelValue::CreateBool(parsedResult);
}

absl::Status RegisterExtraFuncs(
    google::api::expr::runtime::CelFunctionRegistry& registry, google::protobuf::Arena* regArena) {
  auto* formatter = google::protobuf::Arena::Create<StringFormat>(regArena);
  auto status = cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder, cel::CelValue>::
      CreateAndRegister(
          "format",
          true,
          [formatter](
              google::protobuf::Arena* arena,
              cel::CelValue::StringHolder format,
              cel::CelValue arg) -> cel::CelValue { return formatter->format(arena, format, arg); },
          &registry);
  if (!status.ok()) {
    return status;
  }
  auto isNanStatus = cel::FunctionAdapter<cel::CelValue, cel::CelValue>::CreateAndRegister(
      "isNan", true, &isNan, &registry);
  if (!isNanStatus.ok()) {
    return isNanStatus;
  }
  auto isInfXStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue, cel::CelValue>::CreateAndRegister(
          "isInf", true, &isInfX, &registry);
  if (!isInfXStatus.ok()) {
    return isInfXStatus;
  }
  auto isInfStatus = cel::FunctionAdapter<cel::CelValue, cel::CelValue>::CreateAndRegister(
      "isInf", true, &isInf, &registry);
  if (!isInfStatus.ok()) {
    return isInfStatus;
  }
  auto uniqueStatus = cel::FunctionAdapter<cel::CelValue, cel::CelValue>::CreateAndRegister(
      "unique", true, &unique, &registry);
  if (!uniqueStatus.ok()) {
    return uniqueStatus;
  }
  auto containsStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::BytesHolder, cel::CelValue>::
          CreateAndRegister("contains", true, &contains, &registry);
  if (!containsStatus.ok()) {
    return containsStatus;
  }
  auto isIpvStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder, cel::CelValue>::
          CreateAndRegister("isIp", true, &isIPvX, &registry);
  if (!isIpvStatus.ok()) {
    return isIpvStatus;
  }
  auto isIpStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isIp", true, &isIP, &registry);
  if (!isIpStatus.ok()) {
    return isIpStatus;
  }
  auto isIpPrefixStatusXY = cel::
      FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder, cel::CelValue, cel::CelValue>::
          CreateAndRegister("isIpPrefix", true, &isIpPrefixXY, &registry);
  if (!isIpPrefixStatusXY.ok()) {
    return isIpPrefixStatusXY;
  }
  auto isIpPrefixStatusX =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder, cel::CelValue>::
          CreateAndRegister("isIpPrefix", true, &isIpPrefixX, &registry);
  if (!isIpPrefixStatusX.ok()) {
    return isIpPrefixStatusX;
  }
  auto isIpPrefixStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isIpPrefix", true, &isIpPrefix, &registry);
  if (!isIpPrefixStatus.ok()) {
    return isIpPrefixStatus;
  }
  auto startsWithStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::BytesHolder, cel::CelValue>::
          CreateAndRegister("startsWith", true, &startsWith, &registry);
  if (!startsWithStatus.ok()) {
    return startsWithStatus;
  }
  auto endsWithStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::BytesHolder, cel::CelValue>::
          CreateAndRegister("endsWith", true, &endsWith, &registry);
  if (!endsWithStatus.ok()) {
    return endsWithStatus;
  }
  auto isHostnameStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isHostname", true, &isHostname, &registry);
  if (!isHostnameStatus.ok()) {
    return isHostnameStatus;
  }
  auto isEmailStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isEmail", true, &isEmail, &registry);
  if (!isEmailStatus.ok()) {
    return isEmailStatus;
  }
  auto isUriStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isUri", true, &isUri, &registry);
  if (!isUriStatus.ok()) {
    return isUriStatus;
  }
  auto isUriRefStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isUriRef", true, &isUriRef, &registry);
  if (!isUriRefStatus.ok()) {
    return isUriStatus;
  }
  auto isHostAndPortStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder, cel::CelValue>::
          CreateAndRegister("isHostAndPort", true, &isHostAndPort, &registry);
  if (!isHostAndPortStatus.ok()) {
    return isHostAndPortStatus;
  }
  return absl::OkStatus();
}
} // namespace buf::validate::internal
