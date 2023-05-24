#pragma once

#include <string>

#include "buf/validate/expression.pb.h"
#include "eval/public/activation.h"
#include "eval/public/cel_expression.h"
#include "src/google/protobuf/arena.h"

namespace buf::validate::internal {

// A compiled constraint expression.
struct CompiledConstraint {
  buf::validate::Constraint constraint;
  std::unique_ptr<google::api::expr::runtime::CelExpression> expr;
};

struct ConstraintContext {
  std::string fieldPath;
  bool failFast;
  google::protobuf::Arena* arena;
  Violations violations;
};

// A set of constraints that can be evaluated together.
class ConstraintSet {
 public:
  absl::Status Validate(ConstraintContext& ctx, google::api::expr::runtime::Activation& activation);

  absl::Status Add(
      google::api::expr::runtime::CelExpressionBuilder& builder, Constraint constraint);

  [[nodiscard]] const std::vector<CompiledConstraint>& getExprs() const { return exprs_; }

  void setRules(google::api::expr::runtime::CelValue rules) { rules_ = rules; }
  [[nodiscard]] const google::api::expr::runtime::CelValue& getRules() const { return rules_; }
  [[nodiscard]] google::api::expr::runtime::CelValue& getRules() { return rules_; }

 private:
  google::api::expr::runtime::CelValue rules_;
  std::vector<CompiledConstraint> exprs_;
};

// Creates a new expression builder suitable for creating constraints.
absl::StatusOr<std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder>>
NewConstraintBuilder();

using MessageConstraint =
    std::function<absl::Status(ConstraintContext& ctx, const google::protobuf::Message&)>;

using MessageConstraints = absl::StatusOr<std::vector<MessageConstraint>>;

MessageConstraints NewMessageConstraints(
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::Descriptor* descriptor);

} // namespace buf::validate::internal
