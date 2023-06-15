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

#include "buf/validate/internal/constraints.h"

#include "absl/status/statusor.h"
#include "buf/validate/internal/extra_func.h"
#include "buf/validate/priv/private.pb.h"
#include "buf/validate/validate.pb.h"
#include "eval/public/builtin_func_registrar.h"
#include "eval/public/cel_expr_builder_factory.h"
#include "eval/public/cel_value.h"
#include "eval/public/containers/field_access.h"
#include "eval/public/containers/field_backed_list_impl.h"
#include "eval/public/containers/field_backed_map_impl.h"
#include "eval/public/structs/cel_proto_wrapper.h"
#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"

namespace buf::validate::internal {
namespace cel = google::api::expr;

absl::StatusOr<std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder>>
NewConstraintBuilder(google::protobuf::Arena* arena) {
  cel::runtime::InterpreterOptions options;
  options.enable_qualified_type_identifiers = true;
  options.enable_timestamp_duration_overflow_errors = true;
  options.enable_heterogeneous_equality = true;
  options.enable_empty_wrapper_null_unboxing = true;
  options.enable_regex_precompilation = true;
  options.constant_folding = true;
  options.constant_arena = arena;

  std::unique_ptr<cel::runtime::CelExpressionBuilder> builder =
      cel::runtime::CreateCelExpressionBuilder(options);
  auto register_status = cel::runtime::RegisterBuiltinFunctions(builder->GetRegistry(), options);
  if (!register_status.ok()) {
    return register_status;
  }
  register_status = RegisterExtraFuncs(*builder->GetRegistry(), arena);
  if (!register_status.ok()) {
    return register_status;
  }
  return builder;
}

absl::Status MessageConstraintRules::Validate(
    ConstraintContext& ctx, const google::protobuf::Message& message) const {
  google::api::expr::runtime::Activation activation;
  activation.InsertValue("this", cel::runtime::CelProtoWrapper::CreateMessage(&message, ctx.arena));
  return ValidateCel(ctx, "", activation);
}

absl::Status FieldConstraintRules::Validate(
    ConstraintContext& ctx, const google::protobuf::Message& message) const {
  google::api::expr::runtime::Activation activation;
  cel::runtime::CelValue result;
  std::string subPath;
  if (field_->is_map()) {
    result = cel::runtime::CelValue::CreateMap(
        google::protobuf::Arena::Create<cel::runtime::FieldBackedMapImpl>(
            ctx.arena, &message, field_, ctx.arena));
    if (result.MapOrDie()->size() == 0) {
      if (ignoreEmpty_) {
        return absl::OkStatus();
      } else if (required_) {
        auto& violation = *ctx.violations.add_violations();
        *violation.mutable_constraint_id() = "required";
        *violation.mutable_message() = "value is required";
        *violation.mutable_field_path() = field_->name();
      }
    }
  } else if (field_->is_repeated()) {
    result = cel::runtime::CelValue::CreateList(
        google::protobuf::Arena::Create<cel::runtime::FieldBackedListImpl>(
            ctx.arena, &message, field_, ctx.arena));
    if (result.ListOrDie()->size() == 0) {
      if (ignoreEmpty_) {
        return absl::OkStatus();
      } else if (required_) {
        auto& violation = *ctx.violations.add_violations();
        *violation.mutable_constraint_id() = "required";
        *violation.mutable_message() = "value is required";
        *violation.mutable_field_path() = field_->name();
      }
    }
  } else {
    if (!message.GetReflection()->HasField(message, field_)) {
      if (required_) {
        auto& violation = *ctx.violations.add_violations();
        *violation.mutable_constraint_id() = "required";
        *violation.mutable_message() = "value is required";
        *violation.mutable_field_path() = field_->name();
        return absl::OkStatus();
      } else if (
          ignoreEmpty_ || field_->containing_oneof() != nullptr ||
          field_->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
        return absl::OkStatus();
      }
    }

    if (ignoreEmpty_ && field_->containing_oneof() != nullptr) {
      // If the field is in a oneof, we have to manually check if the value is 'empty'.
      switch (field_->type()) {
        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
          if (message.GetReflection()->GetString(message, field_).empty()) {
            return absl::OkStatus();
          }
          break;
        }
        case google::protobuf::FieldDescriptor::TYPE_STRING: {
          if (message.GetReflection()->GetString(message, field_).empty()) {
            return absl::OkStatus();
          }
          break;
        }
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::TYPE_INT32:
          if (message.GetReflection()->GetInt32(message, field_) == 0) {
            return absl::OkStatus();
          }
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::TYPE_INT64: {
          if (message.GetReflection()->GetInt64(message, field_) == 0) {
            return absl::OkStatus();
          }
          break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
          if (message.GetReflection()->GetUInt32(message, field_) == 0) {
            return absl::OkStatus();
          }
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::TYPE_UINT64: {
          if (message.GetReflection()->GetUInt64(message, field_) == 0) {
            return absl::OkStatus();
          }
          break;
        }
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
          if (message.GetReflection()->GetDouble(message, field_) == 0) {
            return absl::OkStatus();
          }
          break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BOOL: {
          if (!message.GetReflection()->GetBool(message, field_)) {
            return absl::OkStatus();
          }
          break;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
          if (message.GetReflection()->GetEnumValue(message, field_) == 0) {
            return absl::OkStatus();
          }
          break;
        }
        default:
          break;
      }
    }

    if (anyRules_ != nullptr &&
        field_->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      const auto& anyMsg = message.GetReflection()->GetMessage(message, field_);
      auto status = ValidateAny(ctx, field_->name(), anyMsg);
      if (!status.ok()) {
        return status;
      }
    }

    auto status = cel::runtime::CreateValueFromSingleField(&message, field_, ctx.arena, &result);
    if (!status.ok()) {
      return status;
    }
  }
  activation.InsertValue("this", result);
  return ValidateCel(ctx, field_->name(), activation);
}

absl::Status EnumConstraintRules::Validate(
    ConstraintContext& ctx, const google::protobuf::Message& message) const {
  if (auto status = Base::Validate(ctx, message); ctx.shouldReturn(status)) {
    return status;
  }
  if (definedOnly_) {
    auto value = message.GetReflection()->GetEnumValue(message, field_);
    if (field_->enum_type()->FindValueByNumber(value) == nullptr) {
      auto& violation = *ctx.violations.add_violations();
      *violation.mutable_constraint_id() = "enum.defined_only";
      *violation.mutable_message() = "enum value must be defined";
      *violation.mutable_field_path() = field_->name();
    }
  }
  return absl::OkStatus();
}

absl::Status RepeatedConstraintRules::Validate(
    ConstraintContext& ctx, const google::protobuf::Message& message) const {
  auto status = Base::Validate(ctx, message);
  if (ctx.shouldReturn(status) || itemRules_ == nullptr) {
    return status;
  }
  // Validate each item.
  auto& list = *google::protobuf::Arena::Create<cel::runtime::FieldBackedListImpl>(
      ctx.arena, &message, field_, ctx.arena);
  for (int i = 0; i < list.size(); i++) {
    cel::runtime::Activation activation;
    activation.InsertValue("this", list[i]);
    int pos = ctx.violations.violations_size();
    status = itemRules_->ValidateCel(ctx, "", activation);
    if (itemRules_->getAnyRules() != nullptr) {
      const auto& anyMsg = message.GetReflection()->GetRepeatedMessage(message, field_, i);
      status = itemRules_->ValidateAny(ctx, "", anyMsg);
    }
    if (ctx.violations.violations_size() > pos) {
      ctx.prefixFieldPath(absl::StrCat(field_->name(), "[", i, "]"), pos);
    }
    if (ctx.shouldReturn(status)) {
      return status;
    }
  }
  return absl::OkStatus();
}

absl::Status MapConstraintRules::Validate(
    ConstraintContext& ctx, const google::protobuf::Message& message) const {
  auto status = Base::Validate(ctx, message);
  if (!status.ok() || (keyRules_ == nullptr && valueRules_ == nullptr)) {
    return status;
  }
  cel::runtime::FieldBackedMapImpl mapVal(&message, field_, ctx.arena);
  cel::runtime::Activation activation;
  const auto* keyField = field_->message_type()->FindFieldByName("key");
  auto keys_or = mapVal.ListKeys();
  if (!keys_or.ok()) {
    return keys_or.status();
  }
  const auto& keys = *std::move(keys_or).value();
  for (int i = 0; i < mapVal.size(); i++) {
    const auto& elemMsg = message.GetReflection()->GetRepeatedMessage(message, field_, i);
    // std::string elemPath = absl::StrCat(subPath, "[", makeMapKeyString(elemMsg, keyField), "]");
    size_t pos = ctx.violations.violations_size();
    auto key = keys[i];
    if (keyRules_ != nullptr) {
      activation.InsertValue("this", key);
      status = keyRules_->ValidateCel(ctx, "", activation);
      if (!status.ok()) {
        return status;
      }
      activation.RemoveValueEntry("this");
    }
    if (valueRules_ != nullptr) {
      auto value = mapVal[key];
      if (value.has_value()) {
        activation.InsertValue("this", *value);
        status = valueRules_->ValidateCel(ctx, "", activation);
        if (!status.ok()) {
          return status;
        }
        activation.RemoveValueEntry("this");
      }
    }
    if (ctx.violations.violations_size() > pos) {
      ctx.prefixFieldPath(
          absl::StrCat(field_->name(), "[", makeMapKeyString(elemMsg, keyField), "]"), pos);
      if (ctx.failFast) {
        return absl::OkStatus();
      }
    }
  }
  return absl::OkStatus();
}

absl::Status FieldConstraintRules::ValidateAny(
    ConstraintContext& ctx,
    std::string_view fieldPath,
    const google::protobuf::Message& anyMsg) const {
  const auto* typeUriField = anyMsg.GetDescriptor()->FindFieldByName("type_url");
  if (typeUriField == nullptr ||
      typeUriField->type() != google::protobuf::FieldDescriptor::TYPE_STRING) {
    return absl::InvalidArgumentError("expected Any");
  }
  const auto& typeUri = anyMsg.GetReflection()->GetString(anyMsg, typeUriField);
  if (anyRules_->in_size() > 0) {
    // Must be in the list of allowed types.
    bool found = false;
    for (const auto& allowed : anyRules_->in()) {
      if (allowed == typeUri) {
        found = true;
        break;
      }
    }
    if (!found) {
      auto& violation = *ctx.violations.add_violations();
      *violation.mutable_constraint_id() = "any.in";
      *violation.mutable_message() = "type URL must be in the allow list";
      *violation.mutable_field_path() = fieldPath;
    }
  }
  for (const auto& block : anyRules_->not_in()) {
    if (block == typeUri) {
      auto& violation = *ctx.violations.add_violations();
      *violation.mutable_constraint_id() = "any.not_in";
      *violation.mutable_message() = "type URL must not be in the block list";
      *violation.mutable_field_path() = fieldPath;
      break;
    }
  }
  return absl::OkStatus();
}

absl::Status OneofConstraintRules::Validate(
    buf::validate::internal::ConstraintContext& ctx,
    const google::protobuf::Message& message) const {
  if (required_) {
    if (!message.GetReflection()->HasOneof(message, oneof_)) {
      auto& violation = *ctx.violations.add_violations();
      *violation.mutable_constraint_id() = "required";
      *violation.mutable_message() = "exactly one field is required in oneof";
      *violation.mutable_field_path() = oneof_->name();
    }
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal
