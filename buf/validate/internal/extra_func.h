// Copyright 2023-2025 Buf Technologies, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

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

// define for testing
bool IsIpv4Prefix(const std::string_view to_validate, bool strict);
bool IsIpv6Prefix(const std::string_view to_validate, bool strict);
bool IsIpPrefix(const std::string_view to_validate, bool strict);

} // namespace buf::validate::internal
