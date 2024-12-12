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

#include <string>
#include <string_view>
#include <utility>

#include "buf/validate/internal/cel_constraint_rules.h"
#include "buf/validate/validate.pb.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message.h"
#include "src/google/protobuf/descriptor.h"

namespace buf::validate::internal {

class MessageConstraintRules final : public CelConstraintRules {
 public:
  MessageConstraintRules() = default;

  absl::Status Validate(
      ConstraintContext& ctx, const google::protobuf::Message& message) const override;
};

class FieldConstraintRules : public CelConstraintRules {
 public:
  FieldConstraintRules(
      const google::protobuf::FieldDescriptor* desc,
      const FieldConstraints& field,
      const AnyRules* anyRules = nullptr)
      : fieldConstraints_(field),
        field_(desc),
        mapEntryField_(desc->containing_type()->options().map_entry()),
        ignoreEmpty_(field.ignore() == IGNORE_IF_DEFAULT_VALUE ||
                     field.ignore() == IGNORE_IF_UNPOPULATED ||
                     field.ignore_empty() ||
                     (desc->has_presence() && !mapEntryField_)),
        ignoreDefault_(field.ignore() == IGNORE_IF_DEFAULT_VALUE &&
                       (desc->has_presence() && !mapEntryField_)),
        required_(field.required()),
        anyRules_(anyRules) {}

  absl::Status Validate(
      ConstraintContext& ctx, const google::protobuf::Message& message) const override;

  absl::Status ValidateAny(
      ConstraintContext& ctx,
      const ProtoField& field,
      const google::protobuf::Message& anyMsg) const;

  [[nodiscard]] const AnyRules* getAnyRules() const { return anyRules_; }

  [[nodiscard]] bool getIgnoreEmpty() const { return ignoreEmpty_; }

  [[nodiscard]] bool getIgnoreDefault() const { return ignoreDefault_; }

 protected:
  const FieldConstraints& fieldConstraints_;
  const google::protobuf::FieldDescriptor* field_ = nullptr;
  bool mapEntryField_ = false;
  bool ignoreEmpty_ = false;
  bool ignoreDefault_ = false;
  bool required_ = false;
  const AnyRules* anyRules_ = nullptr;
};

class EnumConstraintRules : public FieldConstraintRules {
  using Base = FieldConstraintRules;

 public:
  EnumConstraintRules(const google::protobuf::FieldDescriptor* desc, const FieldConstraints& field)
      : Base(desc, field), definedOnly_(field.enum_().defined_only()) {}

  absl::Status Validate(
      ConstraintContext& ctx, const google::protobuf::Message& message) const override;

 private:
  bool definedOnly_;
};

class RepeatedConstraintRules : public FieldConstraintRules {
  using Base = FieldConstraintRules;

 public:
  RepeatedConstraintRules(
      const google::protobuf::FieldDescriptor* desc,
      const FieldConstraints& field,
      std::unique_ptr<FieldConstraintRules> itemRules)
      : Base(desc, field), itemRules_(std::move(itemRules)) {}

  absl::Status Validate(
      ConstraintContext& ctx, const google::protobuf::Message& message) const override;

 private:
  std::unique_ptr<FieldConstraintRules> itemRules_;
};

class MapConstraintRules : public FieldConstraintRules {
  using Base = FieldConstraintRules;

 public:
  MapConstraintRules(
      const google::protobuf::FieldDescriptor* desc,
      const FieldConstraints& field,
      std::unique_ptr<FieldConstraintRules> keyRules,
      std::unique_ptr<FieldConstraintRules> valueRules)
      : Base(desc, field), keyRules_(std::move(keyRules)), valueRules_(std::move(valueRules)) {}

  absl::Status Validate(
      ConstraintContext& ctx, const google::protobuf::Message& message) const override;

 private:
  std::unique_ptr<FieldConstraintRules> keyRules_;
  std::unique_ptr<FieldConstraintRules> valueRules_;
};

// A set of constraints that share the same 'rule' value.
class OneofConstraintRules : public ConstraintRules {
  using Base = ConstraintRules;

 public:
  OneofConstraintRules(const google::protobuf::OneofDescriptor* desc, const OneofConstraints& oneof)
      : oneof_(desc), required_(oneof.required()) {}

  absl::Status Validate(
      ConstraintContext& ctx, const google::protobuf::Message& message) const override;

 private:
  const google::protobuf::OneofDescriptor* oneof_ = nullptr;
  bool required_ = false;
};

// Creates a new expression builder suitable for creating constraints.
absl::StatusOr<std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder>>
NewConstraintBuilder(google::protobuf::Arena* arena);

inline auto fieldPathElement(const google::protobuf::FieldDescriptor* fieldDescriptor) -> FieldPathElement {
  FieldPathElement element;
  element.set_field_number(fieldDescriptor->number());
  element.set_field_type(
      static_cast<::google::protobuf::FieldDescriptorProto_Type>(fieldDescriptor->type()));
  if (fieldDescriptor->is_extension()) {
    *element.mutable_field_name() = absl::StrCat("[", fieldDescriptor->full_name(), "]");
  } else {
    *element.mutable_field_name() = fieldDescriptor->name();
  }
  return element;
}

template<typename ProtoMessage, int number>
auto staticFieldPathElement() -> FieldPathElement {
  // This is thread-safe: C++11 guarantees static local initialization to be done only once and is
  // synchronized automatically.
  static const FieldPathElement element =
      fieldPathElement(ProtoMessage::descriptor()->FindFieldByNumber(number));
  return element;
}

inline auto oneofPathElement(const google::protobuf::OneofDescriptor& oneofDescriptor)
    -> FieldPathElement {
  FieldPathElement element;
  *element.mutable_field_name() = oneofDescriptor.name();
  return element;
}

inline absl::Status setPathElementMapKey(
    FieldPathElement* element,
    const google::protobuf::Message& message,
    const google::protobuf::FieldDescriptor* keyField,
    const google::protobuf::FieldDescriptor* valueField) {
  using Type = google::protobuf::FieldDescriptor::Type;
  element->set_key_type(
      static_cast<::google::protobuf::FieldDescriptorProto_Type>(keyField->type()));
  element->set_value_type(
      static_cast<::google::protobuf::FieldDescriptorProto_Type>(valueField->type()));
  switch (keyField->type()) {
    case Type::TYPE_BOOL:
      element->set_bool_key(message.GetReflection()->GetBool(message, keyField));
      break;
    case Type::TYPE_INT32:
    case Type::TYPE_SFIXED32:
    case Type::TYPE_SINT32:
      element->set_int_key(message.GetReflection()->GetInt32(message, keyField));
      break;
    case Type::TYPE_INT64:
    case Type::TYPE_SFIXED64:
    case Type::TYPE_SINT64:
      element->set_int_key(message.GetReflection()->GetInt64(message, keyField));
      break;
    case Type::TYPE_UINT32:
    case Type::TYPE_FIXED32:
      element->set_uint_key(message.GetReflection()->GetUInt32(message, keyField));
      break;
    case Type::TYPE_UINT64:
    case Type::TYPE_FIXED64:
      element->set_uint_key(message.GetReflection()->GetUInt64(message, keyField));
      break;
    case Type::TYPE_STRING:
      *element->mutable_string_key() = message.GetReflection()->GetString(message, keyField);
      break;
    default:
      return absl::InternalError(absl::StrCat("unexpected map key type ", keyField->type_name()));
  }
  return {};
}

} // namespace buf::validate::internal
