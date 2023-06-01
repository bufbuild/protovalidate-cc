#include "buf/validate/internal/extra_func.h"

#include <algorithm>
#include <string>

#include "absl/strings/match.h"
#include "buf/validate/internal/string_format.h"
#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "eval/public/containers/container_backed_map_impl.h"
#include "google/protobuf/arena.h"
#include "re2/re2.h"

namespace buf::validate::internal {

struct Url {
 public:
  std::string_view scheme, host, path, queryString;

  // Parse parses a URL ref the context of the receiver. The provided URL
  // may be relative or absolute.
  static Url parse(std::string_view ref) {
    Url result;
    if (ref.empty()) {
      return result;
    }
    std::string_view remainder(ref);
    if (absl::StrContains(ref, "://")) {
      std::vector<std::string_view> split = absl::StrSplit(ref, "://");
      result.scheme = split[0];
      std::vector<std::string_view> hostSplit = absl::StrSplit(split[1], absl::MaxSplits('/', 1));
      result.host = hostSplit[0];
      remainder = absl::StrCat("/", hostSplit[1]);
    }
    std::vector<std::string_view> querySplit = absl::StrSplit(remainder, absl::MaxSplits('?', 1));
    if (querySplit.size() == 2) {
      result.queryString = querySplit[1];
    }
    result.path = querySplit[0];
    return result;
  }

  bool isUri() const { return !scheme.empty() && !host.empty(); }

  bool isUriRef() const {
    if (!isValidPath(path)) {
      return false;
    }
    return !scheme.empty() && !host.empty() || !path.empty();
  }

  static bool isValidPath(const std::string_view& path) {
    if (path == "/") {
      return true;
    }
    std::string stringPath(path);
    /**
     * ^: Matches the start of the string.
     * \/: Matches the forward slash ("/") character.
     * [\w\/\-\.]*: Matches zero or more occurrences of the following characters:
     * \w: Matches any alphanumeric character (A-Z, a-z, 0-9) or underscore (_).
     * \/: Matches the forward slash ("/") character.
     * \-: Matches the hyphen ("-") character.
     * \.: Matches the period (dot, ".") character.
     * $: Matches the end of the string.
     */
    re2::RE2 pathPattern(R"(^\/[\w\/\-\.]*$)");
    return re2::RE2::FullMatch(stringPath, pathPattern);
  }
};

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

cel::CelValue isUri(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  auto parsed = Url::parse(lhs.value());
  return cel::CelValue::CreateBool(parsed.isUri());
}

cel::CelValue isUriRef(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  auto parsed = Url::parse(lhs.value());
  return cel::CelValue::CreateBool(parsed.isUriRef());
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
  auto isURIStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isUri", true, &isUri, &registry);
  if (!isURIStatus.ok()) {
    return isURIStatus;
  }
  auto isURIRefStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isUriRef", true, &isUriRef, &registry);
  if (!isURIRefStatus.ok()) {
    return isURIStatus;
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal
