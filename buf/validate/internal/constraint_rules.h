// Copyright 2023 Buf Technologies, Inc.
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

#include "absl/status/status.h"
#include "buf/validate/expression.pb.h"
#include "eval/public/cel_value.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message.h"

namespace buf::validate::internal {
struct ConstraintContext {
  ConstraintContext() : failFast(false), arena(nullptr) {}
  ConstraintContext(const ConstraintContext&) = delete;
  void operator=(const ConstraintContext&) = delete;

  bool failFast;
  google::protobuf::Arena* arena;
  Violations violations;

  [[nodiscard]] bool shouldReturn(const absl::Status status) {
    return !status.ok() || (failFast && violations.violations_size() > 0);
  }

  void prefixFieldPath(std::string_view prefix, int start) {
    for (int i = start; i < violations.violations_size(); i++) {
      auto* violation = violations.mutable_violations(i);
      if (violation->field_path().empty()) {
        *violation->mutable_field_path() = prefix;
      } else {
        violation->set_field_path(absl::StrCat(prefix, ".", violation->field_path()));
      }
    }
  }
};

class ConstraintRules {
 public:
  ConstraintRules() = default;
  virtual ~ConstraintRules() = default;

  ConstraintRules(const ConstraintRules&) = delete;
  void operator=(const ConstraintRules&) = delete;

  virtual absl::Status Validate(
      ConstraintContext& ctx, const google::protobuf::Message& message) const = 0;
};

} // namespace buf::validate::internal
