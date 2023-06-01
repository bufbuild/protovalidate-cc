#pragma once

#include <arpa/inet.h>

#include <functional>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "absl/status/status.h"
#include "eval/public/cel_function_registry.h"
#include "re2/re2.h"
#include "src/google/protobuf/message.h"

namespace buf::validate::internal {

absl::Status RegisterExtraFuncs(
    google::api::expr::runtime::CelFunctionRegistry& registry, google::protobuf::Arena* regArena);

} // namespace buf::validate::internal