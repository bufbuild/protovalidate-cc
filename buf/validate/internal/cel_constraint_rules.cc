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

#include "buf/validate/internal/cel_constraint_rules.h"

#include "common/values/struct_value.h"
#include "eval/public/containers/field_access.h"
#include "eval/public/containers/field_backed_list_impl.h"
#include "eval/public/containers/field_backed_map_impl.h"
#include "eval/public/structs/cel_proto_wrapper.h"
#include "parser/parser.h"

namespace buf::validate::internal {
namespace cel = google::api::expr;

namespace {

absl::Status ProcessConstraint(
    ConstraintContext& ctx,
    const google::api::expr::runtime::BaseActivation& activation,
    const CompiledConstraint& expr) {
  auto result_or= expr.expr->Evaluate(activation, ctx.arena);
  if (!result_or.ok()) {
    return result_or.status();
  }
  cel::runtime::CelValue result = std::move(result_or).value();
  if (result.IsBool()) {
    if (!result.BoolOrDie()) {
      // Add violation with the constraint message.
      Violation violation;
      violation.set_message(expr.constraint.message());
      violation.set_constraint_id(expr.constraint.id());
      if (expr.rulePath.has_value()) {
        *violation.mutable_rule() = *expr.rulePath;
      }
      ctx.violations.emplace_back(violation, absl::nullopt, absl::nullopt);
    }
  } else if (result.IsString()) {
    if (!result.StringOrDie().value().empty()) {
      // Add violation with custom message.
      Violation violation;
      violation.set_message(std::string(result.StringOrDie().value()));
      violation.set_constraint_id(expr.constraint.id());
      if (expr.rulePath.has_value()) {
        *violation.mutable_rule() = *expr.rulePath;
      }
      ctx.violations.emplace_back(violation, absl::nullopt, absl::nullopt);
    }
  } else if (result.IsError()) {
    const cel::runtime::CelError& error = *result.ErrorOrDie();
    return {absl::StatusCode::kInvalidArgument, error.message()};
  } else {
    return {absl::StatusCode::kInvalidArgument, "invalid result type"};
  }
  return absl::OkStatus();
}

cel::runtime::CelValue ProtoFieldToCelValue(
    const google::protobuf::Message* message,
    const google::protobuf::FieldDescriptor* field,
    google::protobuf::Arena* arena) {
  if (field->is_map()) {
    return cel::runtime::CelValue::CreateMap(
        google::protobuf::Arena::Create<cel::runtime::FieldBackedMapImpl>(
            arena, message, field, arena));
  } else if (field->is_repeated()) {
    return cel::runtime::CelValue::CreateList(
        google::protobuf::Arena::Create<cel::runtime::FieldBackedListImpl>(
            arena, message, field, arena));
  } else if (cel::runtime::CelValue result;
             cel::runtime::CreateValueFromSingleField(message, field, arena, &result).ok()) {
    return result;
  }
  return cel::runtime::CelValue::CreateNull();
}

} // namespace

absl::Status CelConstraintRules::Add(
    google::api::expr::runtime::CelExpressionBuilder& builder,
    Constraint constraint,
    absl::optional<FieldPath> rulePath,
    const google::protobuf::FieldDescriptor* rule) {
  auto pexpr_or = cel::parser::Parse(constraint.expression());
  if (!pexpr_or.ok()) {
    return pexpr_or.status();
  }
  cel::v1alpha1::ParsedExpr pexpr = std::move(pexpr_or).value();
  auto expr_or = builder.CreateExpression(pexpr.mutable_expr(), pexpr.mutable_source_info());
  if (!expr_or.ok()) {
    return expr_or.status();
  }
  std::unique_ptr<cel::runtime::CelExpression> expr = std::move(expr_or).value();
  exprs_.emplace_back(CompiledConstraint{std::move(constraint), std::move(expr), std::move(rulePath), rule});
  return absl::OkStatus();
}

absl::Status CelConstraintRules::Add(
    google::api::expr::runtime::CelExpressionBuilder& builder,
    std::string_view id,
    std::string_view message,
    std::string_view expression,
    absl::optional<FieldPath> rulePath,
    const google::protobuf::FieldDescriptor* rule) {
  Constraint constraint;
  *constraint.mutable_id() = id;
  *constraint.mutable_message() = message;
  *constraint.mutable_expression() = expression;
  return Add(builder, constraint, std::move(rulePath), rule);
}

absl::Status CelConstraintRules::ValidateCel(
    ConstraintContext& ctx,
    google::api::expr::runtime::Activation& activation) const {
  activation.InsertValue("rules", rules_);
  activation.InsertValue("now", cel::runtime::CelValue::CreateTimestamp(absl::Now()));
  absl::Status status = absl::OkStatus();

  for (const auto& expr : exprs_) {
    if (rules_.IsMessage() && expr.rule != nullptr) {
      activation.InsertValue(
          "rule", ProtoFieldToCelValue(rules_.MessageOrDie(), expr.rule, ctx.arena));
    }
    int pos = ctx.violations.size();
    status = ProcessConstraint(ctx, activation, expr);
    if (rules_.IsMessage() && expr.rule != nullptr && ctx.violations.size() > pos) {
      ctx.setRuleValue(ProtoField{rules_.MessageOrDie(), expr.rule}, pos);
    }
    if (ctx.shouldReturn(status)) {
      break;
    }
    activation.RemoveValueEntry("rule");
  }
  activation.RemoveValueEntry("rules");
  return status;
}

void CelConstraintRules::setRules(
    const google::protobuf::Message* rules, google::protobuf::Arena* arena) {
  rules_ = cel::runtime::CelProtoWrapper::CreateMessage(rules, arena);
}

} // namespace buf::validate::internal
