#include "buf/validate/internal/extra_func.h"

#include <algorithm>
#include <string>

#include "buf/validate/internal/string_format.h"
#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "google/protobuf/arena.h"

namespace buf::validate::internal {

struct Url {
 public:
  std::string QueryString, Path, Scheme, Host, Port;

  // Parse parses a URL ref the context of the receiver. The provided URL
  // may be relative or absolute.
  static Url& Parse(std::string_view ref) {
    Url result;

    if (ref.empty()) return result;

    absl::string_view uri_view(ref);

    // get query start
    size_t queryStart = uri_view.find('?');

    // protocol
    size_t protocolEnd = uri_view.find(':');
    if (protocolEnd != absl::string_view::npos) {
      absl::string_view prot = uri_view.substr(0, protocolEnd);
      if (absl::EndsWith(prot, "://")) {
        result.Scheme = std::string(prot.begin(), prot.end());
        protocolEnd += 3; //      ://
      } else protocolEnd = 0; // no protocol
    }

    // host
    size_t hostStart = protocolEnd;
    size_t pathStart = uri_view.find('/', hostStart);

    size_t hostEnd = uri_view.find(':', protocolEnd);
    if (hostEnd != absl::string_view::npos &&
        (pathStart == absl::string_view::npos || hostEnd < pathStart)) {
      result.Host = std::string(uri_view.begin() + hostStart, uri_view.begin() + hostEnd);
      hostEnd++; // Skip ':'
      size_t portEnd = (pathStart != absl::string_view::npos) ? pathStart : queryStart;
      result.Port = std::string(uri_view.begin() + hostEnd, uri_view.begin() + portEnd);
    } else {
      if (pathStart != absl::string_view::npos)
        result.Host = std::string(uri_view.begin() + hostStart, uri_view.begin() + pathStart);
      else result.Host = std::string(uri_view.begin() + hostStart, uri_view.end());
    }

    // path
    if (pathStart != absl::string_view::npos)
      result.Path = std::string(uri_view.begin() + pathStart, uri_view.begin() + queryStart);

    // query
    if (queryStart != absl::string_view::npos)
      result.QueryString = std::string(uri_view.begin() + queryStart, uri_view.end());

    return result;
  }

  // IsAbs reports whether the URL is absolute.
  // Absolute means that it has a non-empty scheme.
  bool IsAbs() const { return IsValid() && !Scheme.empty(); }

  bool IsValid() const
  {
    // Check if the required components are present
    return !Scheme.empty() && !Host.empty();
  }
};

namespace cel = google::api::expr::runtime;

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

cel::CelValue isURI(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  struct Url uri = Url::Parse(lhs.value());

  return cel::CelValue::CreateBool(uri.IsAbs());
}

cel::CelValue isURIRef(google::protobuf::Arena* arena, cel::CelValue::StringHolder lhs) {
  struct Url uri = Url::Parse(lhs.value());
  return cel::CelValue::CreateBool(uri.IsValid());
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
          "isURI", true, &isURI, &registry);
  if (!isURIStatus.ok()) {
    return isURIStatus;
  }
  auto isURIRefStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::CreateAndRegister(
          "isURIRef", true, &isURIRef, &registry);
  if (!isURIRefStatus.ok()) {
    return isURIStatus;
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal
