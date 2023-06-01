#include "buf/validate/internal/extra_func.h"

#include <string>
#include <string_view>

#include "absl/strings/match.h"
#include "buf/validate/internal/string_format.h"
#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "eval/public/containers/container_backed_map_impl.h"
#include "google/protobuf/arena.h"

namespace buf::validate::internal {
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

bool IsHostname(const std::string& to_validate) {
  if (to_validate.length() > 253) {
    return false;
  }
  const re2::RE2 component_regex("^[A-Za-z0-9]+(?:-[A-Za-z0-9]+)*$");
  std::vector<std::string> split = absl::StrSplit(to_validate, '.');
  std::vector<std::string> search = {split.begin(), split.end() - 1};
  if (split.size() < 2) {
    return re2::RE2::FullMatch(split[0], component_regex);
  }
  std::string last = split[split.size() - 1];
  for (const std::string& part : search) {
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
  std::string s(lhs.value());
  return cel::CelValue::CreateBool(IsHostname(s));
}

cel::CelValue isEmail(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  absl::string_view addr = lhs.value();
  absl::string_view::size_type pos = addr.find('<');
  if (pos != absl::string_view::npos) {
    return cel::CelValue::CreateBool(false);
  }

  absl::string_view localPart, domainPart;
  std::vector<std::string> atPos = absl::StrSplit(addr, '@');
  if (!atPos.empty()) {
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
  std::string s(domainPart);
  return cel::CelValue::CreateBool(IsHostname(s));
}

bool IsIpv4(const std::string& to_validate) {
  struct sockaddr_in sa;
  return !(inet_pton(AF_INET, to_validate.c_str(), &sa.sin_addr) < 1);
}

bool IsIpv6(const std::string& to_validate) {
  struct sockaddr_in6 sa_six;
  return !(inet_pton(AF_INET6, to_validate.c_str(), &sa_six.sin6_addr) < 1);
}

bool IsIp(const std::string& to_validate) {
  return IsIpv4(to_validate) || IsIpv6(to_validate);
}

cel::CelValue isIPvX(
    google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs, cel::CelValue rhs) {
  std::string s(lhs.value());
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
