#include "buf/validate/internal/extra_func.h"

#include "buf/validate/internal/string_format.h"
#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "google/protobuf/arena.h"
#include "buf/validate/internal/celext.h"

namespace buf::validate::internal {
    namespace cel = google::api::expr::runtime;

    absl::Status RegisterExtraFuncs(
            google::api::expr::runtime::CelFunctionRegistry &registry, google::protobuf::Arena *regArena) {
        auto *formatter = google::protobuf::Arena::Create<StringFormat>(regArena);
        auto formatStatus = cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder, cel::CelValue>::
        CreateAndRegister(
                "format",
                true,
                [formatter](
                        google::protobuf::Arena *arena,
                        cel::CelValue::StringHolder format,
                        cel::CelValue arg) -> cel::CelValue {
                    if (!arg.IsList()) {
                        auto *error = google::protobuf::Arena::Create<cel::CelError>(
                                arena, absl::StatusCode::kInvalidArgument, "format: expected list");
                        return cel::CelValue::CreateError(error);
                    }
                    const cel::CelList &list = *arg.ListOrDie();
                    std::string_view fmtStr = format.value();
                    auto *result = google::protobuf::Arena::Create<std::string>(arena);
                    auto status = formatter->format(*result, fmtStr, list);
                    if (!status.ok()) {
                        auto *error = google::protobuf::Arena::Create<cel::CelError>(
                                arena, absl::StatusCode::kInvalidArgument, status.message());
                        return cel::CelValue::CreateError(error);
                    }
                    return cel::CelValue::CreateString(result);
                },
                &registry);
        if (!formatStatus.ok()) {
            return formatStatus;
        }
        auto *pCelExt = google::protobuf::Arena::Create<CelExt>(regArena);

        auto hostNameStatus = cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::
        CreateAndRegister(
                "isHostname",
                true,
                [pCelExt](
                        google::protobuf::Arena *arena,
                        cel::CelValue::StringHolder hostname) -> cel::CelValue {
                    auto status = pCelExt->isHostname(hostname.value());
                    return cel::CelValue::CreateBool(status);
                },
                &registry);
        if (!hostNameStatus.ok()) {
            return hostNameStatus;
        }

        auto emailStatus = cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::
        CreateAndRegister(
                "isHostname",
                true,
                [pCelExt](
                        google::protobuf::Arena *arena,
                        cel::CelValue::StringHolder hostname) -> cel::CelValue {
                    auto status = pCelExt->isEmail(hostname.value());
                    return cel::CelValue::CreateBool(status);
                },
                &registry);
        if (!emailStatus.ok()) {
            return emailStatus;
        }

        return absl::OkStatus();
    }

} // namespace buf::validate::internal
