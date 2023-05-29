#pragma once

#include <string>
#include <string_view>

#include "buf/validate/internal/cel_constraint_rules.h"
#include "buf/validate/validate.pb.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message.h"
#include "src/google/protobuf/descriptor.h"

namespace buf::validate::internal {

class MessageConstraintRules final : public CelConstraintRules {
 public:
  MessageConstraintRules() = default;

  virtual absl::Status Validate(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& message) const;
};

class FieldConstraintRules : public CelConstraintRules {
 public:
  FieldConstraintRules(
      const google::protobuf::FieldDescriptor* desc,
      const FieldConstraints& field,
      const AnyRules* anyRules = nullptr)
      : field_(desc),
        ignoreEmpty_(field.ignore_empty()),
        required_(field.required()),
        anyRules_(anyRules) {}

  [[nodiscard]] const google::protobuf::FieldDescriptor* getField() const { return field_; }

  virtual absl::Status Validate(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& message) const;

 protected:
  const google::protobuf::FieldDescriptor* field_ = nullptr;
  bool ignoreEmpty_ = false;
  bool required_ = false;
  const AnyRules* anyRules_ = nullptr;

  absl::Status ValidateAny(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& anyMsg) const;
};

class RepeatedConstraintRules : public FieldConstraintRules {
  using Base = FieldConstraintRules;

 public:
  RepeatedConstraintRules(
      const google::protobuf::FieldDescriptor* desc,
      const FieldConstraints& field,
      std::unique_ptr<FieldConstraintRules> itemRules)
      : Base(desc, field), itemRules_(std::move(itemRules)) {}

  virtual absl::Status Validate(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& message) const;

 private:
  std::unique_ptr<FieldConstraintRules> itemRules_;
};

// A set of constraints that share the same 'rule' value.
class OneofConstraintRules : public ConstraintRules {
  using Base = ConstraintRules;

 public:
  OneofConstraintRules(const google::protobuf::OneofDescriptor* desc, const OneofConstraints& oneof)
      : oneof_(desc), required_(oneof.required()) {}

  virtual absl::Status Validate(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& message) const;

 private:
  const google::protobuf::OneofDescriptor* oneof_ = nullptr;
  bool required_ = false;
};

// Creates a new expression builder suitable for creating constraints.
absl::StatusOr<std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder>>
NewConstraintBuilder(google::protobuf::Arena* arena);

using Constraints = absl::StatusOr<std::vector<std::unique_ptr<ConstraintRules>>>;

Constraints NewMessageConstraints(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::Descriptor* descriptor);

} // namespace buf::validate::internal
