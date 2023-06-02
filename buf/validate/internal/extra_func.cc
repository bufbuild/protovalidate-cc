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

#include "buf/validate/internal/extra_func.h"

#include <algorithm>
#include <string>
#include <string_view>

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "buf/validate/internal/string_format.h"
#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "eval/public/containers/container_backed_map_impl.h"
#include "google/protobuf/arena.h"
#include "re2/re2.h"

namespace buf::validate::internal {

bool isPathValid(const std::path) {
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

bool IsHostname(const std::string_view to_validate) {
  if (to_validate.length() > 253) {
    return false;
  }
  const re2::RE2 component_regex("^[A-Za-z0-9]+(?:-[A-Za-z0-9]+)*$");
  std::vector<std::string_view> split = absl::StrSplit(to_validate, '.');
  std::vector<std::string_view> search = {split.begin(), split.end() - 1};
  if (split.size() < 2) {
    return re2::RE2::FullMatch(split[0], component_regex);
  }
  std::string_view last = split[split.size() - 1];
  for (const std::string_view part : search) {
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
  absl::string_view addr = lhs.value();
  absl::string_view::size_type pos = addr.find('<');
  if (pos != absl::string_view::npos) {
    return cel::CelValue::CreateBool(false);
  }

  absl::string_view localPart, domainPart;
  std::vector<std::string_view> atPos = absl::StrSplit(addr, '@');
  if (atPos.size() > 2) {
    localPart = atPos[0];
    domainPart = atPos[1];
  } else {
    return cel::CelValue::CreateBool(false);
  }

  int localLength = localPart.length();
  if (localLength < 1 || localLength > 64 || localLength + domainPart.length() > 253) {
    return cel::CelValue::CreateBool(false);
  }

  // Validate the hostname
  return cel::CelValue::CreateBool(IsHostname(domainPart));
}

bool IsIpv4(const std::string_view to_validate) {
  struct sockaddr_in sa;
  return !(inet_pton(AF_INET, to_validate.c_str(), &sa.sin_addr) < 1);
}

bool IsIpv6(const std::string_view to_validate) {
  struct sockaddr_in6 sa_six;
  return !(inet_pton(AF_INET6, to_validate.c_str(), &sa_six.sin6_addr) < 1);
}

bool IsIp(const std::string_view to_validate) {
  return IsIpv4(to_validate) || IsIpv6(to_validate);
}

cel::CelValue isIPvX(
    google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs, cel::CelValue rhs) {
  switch (rhs.Int64OrDie()) {
    case 0:
      return cel::CelValue::CreateBool(IsIp(s));
    case 4:
      return cel::CelValue::CreateBool(IsIpv4(s));
    case 6:
      return cel::CelValue::CreateBool(IsIpv6(s));
    default:
      return cel::CelValue::CreateBool(false);
  }
}

cel::CelValue isIP(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  return isIPvX(arena, lhs, cel::CelValue::CreateInt64(0));
}

/**
 * Naive URI validation.
 */
cel::CelValue isUri(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  const std::string_view ref = lhs.value();
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
  const std::string_view ref = lhs.value();
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
    remainder = hostSplit[1];
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
  // TODO(afuller): This should be specialized for the locale.
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
  return absl::OkStatus();
}
} // namespace buf::validate::internal
