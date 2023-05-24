#pragma once

#include <string>

#include "buf/validate/expression.pb.h"
#include "eval/public/cel_expression.h"

namespace buf::validate::internal {

// A compiled constraint expression.
struct CompiledConstraint {
  buf::validate::Constraint constraint;
  std::unique_ptr<google::api::expr::runtime::CelExpression> expr;
};

// A set of constraints that can be evaluated together.
class ConstraintSet {
 public:
  absl::Status Validate(
      const google::api::expr::runtime::BaseActivation& ctx,
      const std::string& fieldPath,
      google::protobuf::Arena* arena,
      std::vector<Violation>& violations);
  absl::Status Add(
      google::api::expr::runtime::CelExpressionBuilder& builder, Constraint constraint);
  [[nodiscard]] const std::vector<CompiledConstraint>& getExprs() const { return exprs_; }

 private:
  std::vector<CompiledConstraint> exprs_;
};

// Creates a new expression builder suitable for creating constraints.
absl::StatusOr<std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder>>
NewConstraintBuilder();

using MessageConstraint =
    std::function<absl::Status(const google::protobuf::Message& message, Violations& violations)>;

using MessageConstraints = absl::StatusOr<std::vector<MessageConstraint>>;

MessageConstraints NewMessageConstraints(
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::Descriptor* descriptor);
} // namespace buf::validate::internal
