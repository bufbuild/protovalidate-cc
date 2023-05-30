#include "buf/validate/internal/extra_func.h"

#include "buf/validate/internal/string_format.h"
#include "eval/public/cel_function_adapter.h"
#include "eval/public/cel_value.h"
#include "google/protobuf/arena.h"

namespace buf::validate::internal {
    namespace cel = google::api::expr::runtime;

    cel::CelValue startsWith(
            google::protobuf::Arena *arena, cel::CelValue::BytesHolder lhs, cel::CelValue rhs) {
        if (!rhs.IsBytes()) {
            auto *error = google::protobuf::Arena::Create<cel::CelError>(
                    arena, absl::StatusCode::kInvalidArgument, "doesnt start with the right thing");
            return cel::CelValue::CreateError(error);
        }
        bool result = absl::StartsWith(lhs.value().data(), rhs.BytesOrDie().value());
        return cel::CelValue::CreateBool(result);
    }

    cel::CelValue endsWith(
            google::protobuf::Arena *arena, cel::CelValue::BytesHolder lhs, cel::CelValue rhs) {
        if (!rhs.IsBytes()) {
            auto *error = google::protobuf::Arena::Create<cel::CelError>(
                    arena, absl::StatusCode::kInvalidArgument, "doesnt end with the right thing");
            return cel::CelValue::CreateError(error);
        }
        bool result = absl::EndsWith(lhs.value().data(), rhs.BytesOrDie().value());
        return cel::CelValue::CreateBool(result);
    }

    cel::CelValue isURICore(google::protobuf::Arena* arena, std::string_view in) {
        std::unordered_set<std::string> allowedSchemes = { "http", "https", "ftp", "ftps" };

        std::vector<std::string> parts = absl::StrSplit(in, ':');
        if (parts.size() < 2) {
            return cel::CelValue::CreateBool(false);
        }

        std::string scheme = absl::AsciiStrToLower(parts[0]);
        if (allowedSchemes.find(scheme) == allowedSchemes.end()) {
            return cel::CelValue::CreateBool(false);
        }

        return cel::CelValue::CreateBool(true);
    }

    cel::CelValue isURI(google::protobuf::Arena *arena, cel::CelValue::StringHolder lhs) {
        std::string_view in = lhs.value();
        std::string schemeStr = absl::StrCat(in.substr(0, in.find(':')), "://");
        std::string::size_type pos = in.find(schemeStr);
        if (pos == std::string::npos) {
            return cel::CelValue::CreateBool(false);
        }

        return isURICore(arena, in);
    }

    cel::CelValue isURIRef(google::protobuf::Arena *arena, cel::CelValue::StringHolder lhs) {
        return isURICore(arena, lhs.value());
    }



    absl::Status RegisterExtraFuncs(
            google::api::expr::runtime::CelFunctionRegistry &registry, google::protobuf::Arena *regArena) {
        // TODO(afuller): This should be specialized for the locale.
        auto *formatter = google::protobuf::Arena::Create<StringFormat>(regArena);
        auto status = cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder, cel::CelValue>::
        CreateAndRegister(
                "format",
                true,
                [formatter](
                        google::protobuf::Arena *arena,
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
                cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::
                CreateAndRegister("isURI", true, &isURI, &registry);
        if (!isURIStatus.ok()) {
            return isURIStatus;
        }
        auto isURIRefStatus =
                cel::FunctionAdapter<cel::CelValue, cel::CelValue::StringHolder>::
                CreateAndRegister("isURIRef", true, &isURIRef, &registry);
        if (!isURIRefStatus.ok()) {
            return isURIStatus;
        }
        return absl::OkStatus();
    }

} // namespace buf::validate::internal
