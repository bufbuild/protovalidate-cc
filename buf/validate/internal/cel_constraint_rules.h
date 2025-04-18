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

#include <string_view>

#include "buf/validate/validate.pb.h"
#include "buf/validate/internal/constraint_rules.h"
#include "eval/public/activation.h"
#include "eval/public/cel_expression.h"
#include "eval/public/cel_value.h"

namespace buf::validate::internal {

// A compiled constraint expression.
struct CompiledConstraint {
  buf::validate::Constraint constraint;
  std::unique_ptr<google::api::expr::runtime::CelExpression> expr;
  const absl::optional<FieldPath> rulePath;
  const google::protobuf::FieldDescriptor* rule;
};

// An abstract base class for constraint with rules that are compiled into CEL expressions.
class CelConstraintRules : public ConstraintRules {
  using Base = ConstraintRules;

 public:
  using Base::Base;

  absl::Status Add(
      google::api::expr::runtime::CelExpressionBuilder& builder,
      Constraint constraint,
      absl::optional<FieldPath> rulePath,
      const google::protobuf::FieldDescriptor* rule);
  absl::Status Add(
      google::api::expr::runtime::CelExpressionBuilder& builder,
      std::string_view id,
      std::string_view message,
      std::string_view expression,
      absl::optional<FieldPath> rulePath,
      const google::protobuf::FieldDescriptor* rule);

  // Validate all the cel rules given the activation that already has 'this' bound.
  absl::Status ValidateCel(
      ConstraintContext& ctx,
      google::api::expr::runtime::Activation& activation) const;

  void setRules(google::api::expr::runtime::CelValue rules) { rules_ = rules; }
  void setRules(const google::protobuf::Message* rules, google::protobuf::Arena* arena);

 protected:
  google::api::expr::runtime::CelValue rules_;
  std::vector<CompiledConstraint> exprs_;
};

} // namespace buf::validate::internal
