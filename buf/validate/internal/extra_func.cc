#include "buf/validate/internal/extra_func.h"

#include <arpa/inet.h>

#include <algorithm>
#include <string>
#include <string_view>

#include "buf/validate/internal/string_format.h"
#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "google/protobuf/arena.h"

namespace buf::validate::internal {
namespace cel = google::api::expr::runtime;

cel::CelValue contains(
    google::protobuf::Arena* arena, cel::CelValue::BytesHolder lhs, cel::CelValue rhs) {
  if (!rhs.IsBytes()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, "doesnt start with the right thing");
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

bool checkHostName(absl::string_view val) {
  if (val.length() > 253) {
    return false;
  }

  std::string s(val);
  absl::string_view delimiter = ".";
  if (s.length() >= delimiter.length() &&
      s.compare(s.length() - delimiter.length(), delimiter.length(), delimiter) == 0) {
    s.substr(0, s.length() - delimiter.length());
  }
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });

  // split isHostname on '.' and validate each part
  size_t pos = 0;
  absl::string_view part;
  while ((pos = s.find(delimiter)) != absl::string_view::npos) {
    part = s.substr(0, pos);
    // if part is empty, longer than 63 chars, or starts/ends with '-', it is invalid
    if (part.empty() || part.length() > 63 || part.front() == '-' || part.back() == '-') {
      return false;
    }
    // for each character lhs part
    for (char ch : part) {
      // if the character is not a-z, 0-9, or '-', it is invalid
      if ((ch < 'a' || ch > 'z') && (ch < '0' || ch > '9') && ch != '-') {
        return false;
      }
    }
    s.erase(0, pos + delimiter.length());
  }

  return true;
}

cel::CelValue isHostname(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  return cel::CelValue::CreateBool(checkHostName(lhs.value()));
}

cel::CelValue isEmail(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  absl::string_view addr = lhs.value();
  absl::string_view::size_type pos = addr.find('<');
  if (pos != absl::string_view::npos) {
    return cel::CelValue::CreateBool(false);
  }

  absl::string_view localPart, domainPart;
  std::vector<std::string> atPos = absl::StrSplit(addr, '@');
  if (atPos[0] != "") {
    localPart = atPos[0];
    domainPart = atPos[1];
  } else {
    return cel::CelValue::CreateBool(false);
  }

  if (localPart.length() < 1 || localPart.length() > 64 || domainPart.length() > 253) {
    return cel::CelValue::CreateBool(false);
  }

  // Validate the hostname
  return cel::CelValue::CreateBool(checkHostName(domainPart));
}

bool validateIP(absl::string_view addr, int64_t ver) {
  struct in_addr addr4;
  struct in6_addr addr6;
  int result;

  switch (ver) {
    case 0:
      return true;
    case 4:
      result = inet_pton(AF_INET, addr.data(), &(addr4.s_addr));
      return result == 1;
    case 6:
      result = inet_pton(AF_INET6, addr.data(), &(addr6.s6_addr));
      return result == 1;
    default:
      return false;
  }
}

cel::CelValue isIP(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  return cel::CelValue::CreateBool(validateIP(lhs.value(), 0));
}

cel::CelValue isIPv4(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  return cel::CelValue::CreateBool(validateIP(lhs.value(), 4));
}

cel::CelValue isIPv6(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  return cel::CelValue::CreateBool(validateIP(lhs.value(), 6));
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
  auto containsStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::BytesHolder, cel::CelValue>::
          CreateAndRegister("contains", true, &contains, &registry);
  if (!containsStatus.ok()) {
    return containsStatus;
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
    return isHostnameStatus;
  }
  auto isIPStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isIP", true, &isIP, &registry);
  if (!isIPStatus.ok()) {
    return isHostnameStatus;
  }
  auto isIPv4Status =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isIPv4", true, &isIPv4, &registry);
  if (!isIPv4Status.ok()) {
    return isHostnameStatus;
  }
  auto isIPv6Status =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isIPv6", true, &isIPv6, &registry);
  if (!isIPv6Status.ok()) {
    return isHostnameStatus;
  }

  return absl::OkStatus();
}

} // namespace buf::validate::internal
