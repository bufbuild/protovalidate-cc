#pragma once

#include <string>
#include <string_view>

#include <buf/validate/validate.pb.h>

#include "buf/validate/expression.pb.h"
#include "eval/public/activation.h"
#include "eval/public/cel_expression.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message.h"
#include "src/google/protobuf/descriptor.h"

namespace buf::validate::internal {

// A compiled constraint expression.
struct CompiledConstraint {
  buf::validate::Constraint constraint;
  std::unique_ptr<google::api::expr::runtime::CelExpression> expr;
};

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
};

// A set of constraints that share the same 'rule' value.
class ConstraintSet {
 public:
  ConstraintSet() = default;
  ConstraintSet(const ConstraintSet&) = delete;
  void operator=(const ConstraintSet&) = delete;

  ConstraintSet(
      const google::protobuf::FieldDescriptor* desc,
      const FieldConstraints& field,
      const AnyRules* anyRules = nullptr)
      : field_(desc),
        ignoreEmpty_(field.ignore_empty()),
        required_(field.required()),
        anyRules_(anyRules) {}
  ConstraintSet(const google::protobuf::OneofDescriptor* desc, const OneofConstraints& oneof)
      : oneof_(desc), required_(oneof.required()) {}

  absl::Status Validate(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      google::api::expr::runtime::Activation& activation) const;

  absl::Status Validate(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& message) const;

  absl::Status Add(
      google::api::expr::runtime::CelExpressionBuilder& builder, Constraint constraint);
  absl::Status Add(
      google::api::expr::runtime::CelExpressionBuilder& builder,
      std::string_view id,
      std::string_view message,
      std::string_view expression);

  [[nodiscard]] const std::vector<CompiledConstraint>& getExprs() const { return exprs_; }

  void setRules(google::api::expr::runtime::CelValue rules) { rules_ = rules; }
  void setRules(const google::protobuf::Message* rules, google::protobuf::Arena* arena);
  [[nodiscard]] const google::api::expr::runtime::CelValue& getRules() const { return rules_; }
  [[nodiscard]] google::api::expr::runtime::CelValue& getRules() { return rules_; }

  [[nodiscard]] const google::protobuf::FieldDescriptor* getField() const { return field_; }

 private:
  google::api::expr::runtime::CelValue rules_;
  std::vector<CompiledConstraint> exprs_;

  // The field to bind to 'this' or null if the entire message should be bound.
  const google::protobuf::FieldDescriptor* field_ = nullptr;
  const google::protobuf::OneofDescriptor* oneof_ = nullptr;
  bool ignoreEmpty_ = false;
  bool required_ = false;

  const AnyRules* anyRules_ = nullptr;

  absl::Status ValidateMessage(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& message) const;

  absl::Status ValidateField(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& message) const;

  absl::Status ValidateOneof(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& message) const;

  absl::Status ValidateAny(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& anyMsg) const;
};

// Creates a new expression builder suitable for creating constraints.
absl::StatusOr<std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder>>
NewConstraintBuilder(google::protobuf::Arena* arena);

using Constraints = absl::StatusOr<std::vector<std::unique_ptr<ConstraintSet>>>;

Constraints NewMessageConstraints(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::Descriptor* descriptor);

} // namespace buf::validate::internal
