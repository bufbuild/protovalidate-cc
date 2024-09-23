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

#include "absl/status/statusor.h"
#include "buf/validate/internal/cel_rules.h"
#include "buf/validate/internal/constraints.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"

namespace buf::validate::internal {

template <typename R>
absl::Status BuildScalarFieldRules(
    FieldConstraintRules& result,
    std::unique_ptr<MessageFactory>& messageFactory,
    bool allowUnknownFields,
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
  return BuildCelRules(messageFactory, allowUnknownFields, arena, builder, rules, result);
}

template <typename R>
absl::StatusOr<std::unique_ptr<FieldConstraintRules>> NewScalarFieldRules(
    std::unique_ptr<MessageFactory>& messageFactory,
    bool allowUnknownFields,
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::FieldDescriptor* field,
    const FieldConstraints& fieldLvl,
    const R& rules,
    google::protobuf::FieldDescriptor::Type expectedType,
    std::string_view wrapperName = "") {
  auto result = std::make_unique<FieldConstraintRules>(field, fieldLvl);
  auto status = BuildScalarFieldRules(
      *result,
      messageFactory,
      allowUnknownFields,
      arena,
      builder,
      field,
      fieldLvl,
      rules,
      expectedType,
      wrapperName);
  if (!status.ok()) {
    return status;
  }
  return result;
}

absl::StatusOr<std::unique_ptr<FieldConstraintRules>> NewFieldRules(
    std::unique_ptr<MessageFactory>& messageFactory,
    bool allowUnknownFields,
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::FieldDescriptor* field,
    const FieldConstraints& fieldLvl);

} // namespace buf::validate::internal
