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
      : field_(desc),
        mapEntryField_(desc->containing_type()->options().map_entry()),
        ignoreEmpty_(field.ignore() == IGNORE_IF_DEFAULT_VALUE ||
                     field.ignore() == IGNORE_IF_UNPOPULATED ||
                     field.ignore_empty() ||
                     (desc->has_presence() && !mapEntryField_)),
        ignoreDefault_(field.ignore() == IGNORE_IF_DEFAULT_VALUE &&
                       (desc->has_presence() && !mapEntryField_)),
        required_(field.required()),
        anyRules_(anyRules) {}

  [[nodiscard]] const google::protobuf::FieldDescriptor* getField() const { return field_; }

  absl::Status Validate(
      ConstraintContext& ctx, const google::protobuf::Message& message) const override;

  absl::Status ValidateAny(
      ConstraintContext& ctx,
      std::string_view fieldName,
      const google::protobuf::Message& anyMsg) const;

  [[nodiscard]] const AnyRules* getAnyRules() const { return anyRules_; }

  [[nodiscard]] bool getIgnoreEmpty() const { return ignoreEmpty_; }

  [[nodiscard]] bool getIgnoreDefault() const { return ignoreDefault_; }

 protected:
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

inline std::string makeMapKeyString(
    const google::protobuf::Message& message, const google::protobuf::FieldDescriptor* keyField) {
  switch (keyField->cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
      return std::to_string(message.GetReflection()->GetInt32(message, keyField));
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
      return std::to_string(message.GetReflection()->GetInt64(message, keyField));
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
      return std::to_string(message.GetReflection()->GetUInt32(message, keyField));
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
      return std::to_string(message.GetReflection()->GetUInt64(message, keyField));
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
      return std::to_string(message.GetReflection()->GetDouble(message, keyField));
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      return std::to_string(message.GetReflection()->GetFloat(message, keyField));
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
      return message.GetReflection()->GetBool(message, keyField) ? "true" : "false";
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
      return message.GetReflection()->GetEnum(message, keyField)->name();
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
      // Quote and escape the string.
      return absl::StrCat(
          "\"", absl::CEscape(message.GetReflection()->GetString(message, keyField)), "\"");
    default:
      return "?";
  }
}

} // namespace buf::validate::internal
