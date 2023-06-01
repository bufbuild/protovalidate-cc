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
#include "google/protobuf/message.h"
#include "re2/re2.h"

namespace buf::validate::internal {

absl::Status RegisterExtraFuncs(
    google::api::expr::runtime::CelFunctionRegistry& registry, google::protobuf::Arena* regArena);

} // namespace buf::validate::internal