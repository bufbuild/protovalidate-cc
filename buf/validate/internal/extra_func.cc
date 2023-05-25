#include "buf/validate/internal/extra_func.h"

#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "google/protobuf/arena.h"

namespace buf::validate::internal {
namespace cel = google::api::expr::runtime;

absl::Status RegisterExtraFuncs(google::api::expr::runtime::CelFunctionRegistry& registry) {
  auto status = cel::
      FunctionAdapter<cel::CelValue::StringHolder, cel::CelValue::StringHolder, cel::CelValue>::
          CreateAndRegister(
              "format",
              true,
              [](google::protobuf::Arena* arena,
                 cel::CelValue::StringHolder format,
                 cel::CelValue arg) -> cel::CelValue::StringHolder {
                auto result =
                    google::protobuf::Arena::Create<std::string>(arena, "format is unimplemented");
                return cel::CelValue::StringHolder(result);
              },
              &registry);
  if (!status.ok()) {
    return status;
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal