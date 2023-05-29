#pragma once

#include "absl/status/statusor.h"
#include "buf/validate/internal/constraint.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"

namespace buf::validate::internal {

template <typename R>
absl::StatusOr<std::unique_ptr<FieldConstraintRules>> BuildScalarFieldRules(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::FieldDescriptor* field,
    const FieldConstraints& fieldLvl,
    const R& rules,
    google::protobuf::FieldDescriptor::Type expectedType,
    std::string_view wrapperName = "") {
  if (field->type() != expectedType) {
    if (field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
      if (field->message_type()->full_name() != wrapperName) {
        return absl::FailedPreconditionError(absl::StrFormat(
            "field type does not match constraint type: %s != %s",
            field->type_name(),
            google::protobuf::FieldDescriptor::TypeName(expectedType)));
      }
    } else {
      return absl::FailedPreconditionError(absl::StrFormat(
          "field type does not match constraint type: %s != %s",
          google::protobuf::FieldDescriptor::TypeName(field->type()),
          google::protobuf::FieldDescriptor::TypeName(expectedType)));
    }
  }
  auto result = std::make_unique<FieldConstraintRules>(field, fieldLvl);
  auto status = BuildCelRules(arena, builder, rules, *result);
  if (!status.ok()) {
    return status;
  }
  return result;
}

absl::StatusOr<std::unique_ptr<FieldConstraintRules>> BuildFieldRules(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::FieldDescriptor* field,
    const FieldConstraints& fieldLvl);

} // namespace buf::validate::internal