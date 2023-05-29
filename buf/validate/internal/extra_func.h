#pragma once

#include "absl/status/status.h"
#include "eval/public/cel_function_registry.h"

namespace buf::validate::internal {

absl::Status RegisterExtraFuncs(
    google::api::expr::runtime::CelFunctionRegistry& registry, google::protobuf::Arena* lhs);

}