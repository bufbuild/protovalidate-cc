#include "buf/validate/internal/extra_func.h"

#include "buf/validate/internal/starts_with.h"
#include "buf/validate/internal/string_format.h"
#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "google/protobuf/arena.h"

namespace buf::validate::internal {
namespace cel = google::api::expr::runtime;

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
  auto* startsWith = google::protobuf::Arena::Create<StartsWith>(regArena);
  auto endsWithStatus = cel::
      FunctionAdapter<cel::CelValue, cel::CelValue::BytesHolder, cel::CelValue>::CreateAndRegister(
          "startsWith",
          true,
          [startsWith](
              google::protobuf::Arena* arena,
              cel::CelValue::BytesHolder lhs,
              cel::CelValue rhs) -> cel::CelValue {
            if (!rhs.IsBytes()) {
              auto* error = google::protobuf::Arena::Create<cel::CelError>(
                  arena,
                  absl::StatusCode::kInvalidArgument,
                  "starts_with: bytes were not provided");
              return cel::CelValue::CreateError(error);
            }
            auto* result = google::protobuf::Arena::Create<std::string>(arena);
            auto suffix = rhs.BytesOrDie().value();
            auto lhsValue = lhs.value();
            if (lhsValue != nullptr && lhsValue.size() >= suffix.size()) {
              auto lhsData = lhsValue.data();
              auto suffixData = suffix.data();
              auto lhsSize = lhsValue.size();
              auto suffixSize = suffix.size();
              if (std::memcmp(lhsData + (lhsSize - suffixSize), suffixData, suffixSize) == 0) {
                return cel::CelValue::CreateString(result);
              }
            }
            auto* error = google::protobuf::Arena::Create<cel::CelError>(
                arena, absl::StatusCode::kInvalidArgument, "starts_with: does not start with");
            return cel::CelValue::CreateError(error);
          },
          &registry);
  if (!endsWithStatus.ok()) {
    return endsWithStatus;
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal
