#pragma once

#include <string_view>

#include "buf/validate/expression.pb.h"
#include "buf/validate/internal/constraint_rules.h"
#include "eval/public/activation.h"
#include "eval/public/cel_expression.h"
#include "eval/public/cel_value.h"

namespace buf::validate::internal {

// A compiled constraint expression.
struct CompiledConstraint {
  buf::validate::Constraint constraint;
  std::unique_ptr<google::api::expr::runtime::CelExpression> expr;
};

// An abstract base class for constraint with rules that are compiled into CEL expressions.
class CelConstraintRules : public ConstraintRules {
  using Base = ConstraintRules;

 public:
  using Base::Base;

  absl::Status Add(
      google::api::expr::runtime::CelExpressionBuilder& builder, Constraint constraint);
  absl::Status Add(
      google::api::expr::runtime::CelExpressionBuilder& builder,
      std::string_view id,
      std::string_view message,
      std::string_view expression);
  [[nodiscard]] const std::vector<CompiledConstraint>& getExprs() const { return exprs_; }

  // Validate all the cel rules given the activation that already has 'this' bound.
  absl::Status ValidateCel(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      google::api::expr::runtime::Activation& activation) const;

  void setRules(google::api::expr::runtime::CelValue rules) { rules_ = rules; }
  void setRules(const google::protobuf::Message* rules, google::protobuf::Arena* arena);
  [[nodiscard]] const google::api::expr::runtime::CelValue& getRules() const { return rules_; }
  [[nodiscard]] google::api::expr::runtime::CelValue& getRules() { return rules_; }

 protected:
  google::api::expr::runtime::CelValue rules_;
  std::vector<CompiledConstraint> exprs_;
};

} // namespace buf::validate::internal
