#include "buf/validate/internal/extra_func.h"

#include "buf/validate/internal/string_format.h"
#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "google/protobuf/arena.h"

namespace buf::validate::internal {
namespace cel = google::api::expr::runtime;

cel::CelValue startsWithTop(
    google::protobuf::Arena* arena, cel::CelValue::BytesHolder lhs, cel::CelValue rhs) {
  if (!rhs.IsBytes()) {
    auto* error = google::protobuf::Arena::Create<cel::CelError>(
        arena, absl::StatusCode::kInvalidArgument, "doesnt start with the right thing");
    return cel::CelValue::CreateError(error);
  }
  bool result = absl::StartsWith(lhs.value().data(), rhs.BytesOrDie().value());
  return cel::CelValue::CreateBool(result);
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
              cel::CelValue arg) -> cel::CelValue {
            if (!arg.IsList()) {
              auto* error = google::protobuf::Arena::Create<cel::CelError>(
                  arena, absl::StatusCode::kInvalidArgument, "format: expected list");
              return cel::CelValue::CreateError(error);
            }
            const cel::CelList& list = *arg.ListOrDie();
            std::string_view fmtStr = format.value();
            auto* result = google::protobuf::Arena::Create<std::string>(arena);
            auto status = formatter->format(*result, fmtStr, list);
            if (!status.ok()) {
              auto* error = google::protobuf::Arena::Create<cel::CelError>(
                  arena, absl::StatusCode::kInvalidArgument, status.message());
              return cel::CelValue::CreateError(error);
            }
            return cel::CelValue::CreateString(result);
          },
          &registry);
  if (!status.ok()) {
    return status;
  }
  auto endsWithStatus =
      cel::FunctionAdapter<cel::CelValue, cel::CelValue::BytesHolder, cel::CelValue>::
          CreateAndRegister("startsWith", true, &startsWithTop, &registry);
  if (!endsWithStatus.ok()) {
    return endsWithStatus;
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal
