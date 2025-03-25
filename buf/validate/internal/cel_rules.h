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

#include "absl/status/status.h"
#include "buf/validate/internal/cel_constraint_rules.h"
#include "buf/validate/internal/constraints.h"
#include "buf/validate/internal/message_factory.h"
#include "buf/validate/validate.pb.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"

namespace buf::validate::internal {

template <typename T>
constexpr int ruleFieldNumber() = delete;

#define MAP_RULES_TO_FIELD_NUMBER(R, fieldNumber) \
  template <>                                     \
  constexpr int ruleFieldNumber<R>() {            \
    return fieldNumber;                           \
  }

MAP_RULES_TO_FIELD_NUMBER(FloatRules, FieldConstraints::kFloatFieldNumber)
MAP_RULES_TO_FIELD_NUMBER(DoubleRules, FieldConstraints::kDoubleFieldNumber)
MAP_RULES_TO_FIELD_NUMBER(Int32Rules, FieldConstraints::kInt32FieldNumber)
MAP_RULES_TO_FIELD_NUMBER(Int64Rules, FieldConstraints::kInt64FieldNumber)
MAP_RULES_TO_FIELD_NUMBER(UInt32Rules, FieldConstraints::kUint32FieldNumber)
MAP_RULES_TO_FIELD_NUMBER(UInt64Rules, FieldConstraints::kUint64FieldNumber)
MAP_RULES_TO_FIELD_NUMBER(SInt32Rules, FieldConstraints::kSint32FieldNumber)
MAP_RULES_TO_FIELD_NUMBER(SInt64Rules, FieldConstraints::kSint64FieldNumber)
MAP_RULES_TO_FIELD_NUMBER(Fixed32Rules, FieldConstraints::kFixed32FieldNumber)
MAP_RULES_TO_FIELD_NUMBER(Fixed64Rules, FieldConstraints::kFixed64FieldNumber)
MAP_RULES_TO_FIELD_NUMBER(SFixed32Rules, FieldConstraints::kSfixed32FieldNumber)
MAP_RULES_TO_FIELD_NUMBER(SFixed64Rules, FieldConstraints::kSfixed64FieldNumber)
MAP_RULES_TO_FIELD_NUMBER(BoolRules, FieldConstraints::kBoolFieldNumber)
MAP_RULES_TO_FIELD_NUMBER(StringRules, FieldConstraints::kStringFieldNumber)
MAP_RULES_TO_FIELD_NUMBER(BytesRules, FieldConstraints::kBytesFieldNumber)
MAP_RULES_TO_FIELD_NUMBER(EnumRules, FieldConstraints::kEnumFieldNumber)
MAP_RULES_TO_FIELD_NUMBER(RepeatedRules, FieldConstraints::kRepeatedFieldNumber)
MAP_RULES_TO_FIELD_NUMBER(MapRules, FieldConstraints::kMapFieldNumber)
MAP_RULES_TO_FIELD_NUMBER(AnyRules, FieldConstraints::kAnyFieldNumber)
MAP_RULES_TO_FIELD_NUMBER(DurationRules, FieldConstraints::kDurationFieldNumber)
MAP_RULES_TO_FIELD_NUMBER(TimestampRules, FieldConstraints::kTimestampFieldNumber)

#undef MAP_RULES_TO_FIELD_NUMBER

template <typename R>
absl::Status BuildCelRules(
    std::unique_ptr<MessageFactory>& messageFactory,
    bool allowUnknownFields,
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const R& rules,
    CelConstraintRules& result) {
  // Look for constraints on the set fields.
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  google::protobuf::Message* reparsedRules{};
  if (messageFactory && rules.unknown_fields().field_count() > 0) {
    reparsedRules = messageFactory->messageFactory()
                        ->GetPrototype(messageFactory->descriptorPool()->FindMessageTypeByName(
                            rules.GetTypeName()))
                        ->New(arena);
    if (!Reparse(*messageFactory, rules, reparsedRules)) {
      reparsedRules = nullptr;
    }
  }
  if (reparsedRules) {
    if (!allowUnknownFields &&
        !reparsedRules->GetReflection()->GetUnknownFields(*reparsedRules).empty()) {
      return absl::FailedPreconditionError(
          absl::StrCat("unknown constraints in ", reparsedRules->GetTypeName()));
    }
    result.setRules(reparsedRules, arena);
    reparsedRules->GetReflection()->ListFields(*reparsedRules, &fields);
  } else {
    if (!allowUnknownFields && !R::GetReflection()->GetUnknownFields(rules).empty()) {
      return absl::FailedPreconditionError(
          absl::StrCat("unknown constraints in ", rules.GetTypeName()));
    }
    result.setRules(&rules, arena);
    R::GetReflection()->ListFields(rules, &fields);
  }
  for (const auto* field : fields) {
    FieldPath rulePath;
    *rulePath.mutable_elements()->Add() = fieldPathElement(field);
    *rulePath.mutable_elements()->Add() =
        staticFieldPathElement<FieldConstraints, ruleFieldNumber<R>()>();
    if (!field->options().HasExtension(buf::validate::predefined)) {
      continue;
    }
    const auto& fieldLvl = field->options().GetExtension(buf::validate::predefined);
    for (const auto& constraint : fieldLvl.cel()) {
      auto status = result.Add(
          builder, constraint.id(), constraint.message(), constraint.expression(), rulePath, field);
      if (!status.ok()) {
        return status;
      }
    }
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal
