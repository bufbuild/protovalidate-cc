#include "buf/validate/internal/constraint.h"

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
    ConstraintContext& ctx,
    std::string_view fieldPath,
    const google::protobuf::Message& message) const {
  google::api::expr::runtime::Activation activation;
  activation.InsertValue("this", cel::runtime::CelProtoWrapper::CreateMessage(&message, ctx.arena));
  return ValidateCel(ctx, fieldPath, activation);
}

absl::Status FieldConstraintRules::Validate(
    ConstraintContext& ctx,
    std::string_view fieldPath,
    const google::protobuf::Message& message) const {
  google::api::expr::runtime::Activation activation;
  cel::runtime::CelValue result;
  std::string subPath;
  if (fieldPath.empty()) {
    fieldPath = field_->name();
  } else {
    subPath = absl::StrCat(fieldPath, ".", field_->name());
    fieldPath = subPath;
  }
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
        *violation.mutable_field_path() = fieldPath;
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
        *violation.mutable_field_path() = fieldPath;
      }
    }
  } else {
    if (!message.GetReflection()->HasField(message, field_)) {
      if (required_) {
        auto& violation = *ctx.violations.add_violations();
        *violation.mutable_constraint_id() = "required";
        *violation.mutable_message() = "value is required";
        *violation.mutable_field_path() = fieldPath;
        return absl::OkStatus();
      } else if (
          ignoreEmpty_ || field_->containing_oneof() != nullptr ||
          field_->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
        return absl::OkStatus();
      }
    } else if (
        anyRules_ != nullptr &&
        field_->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      const auto& anyMsg = message.GetReflection()->GetMessage(message, field_);
      auto status = ValidateAny(ctx, fieldPath, anyMsg);
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
  return ValidateCel(ctx, fieldPath, activation);
}

absl::Status RepeatedConstraintRules::Validate(
    ConstraintContext& ctx,
    std::string_view fieldPath,
    const google::protobuf::Message& message) const {
  auto status = Base::Validate(ctx, fieldPath, message);
  if (!status.ok() || itemRules_ == nullptr) {
    return status;
  }
  // Validate each item.
  std::string subPath =
      fieldPath.empty() ? field_->name() : absl::StrCat(fieldPath, ".", field_->name());
  auto& list = *google::protobuf::Arena::Create<cel::runtime::FieldBackedListImpl>(
      ctx.arena, &message, field_, ctx.arena);
  for (int i = 0; i < list.size(); i++) {
    std::string elemPath = absl::StrCat(subPath, "[", i, "]");
    cel::runtime::Activation activation;
    activation.InsertValue("this", list[i]);
    status = itemRules_->ValidateCel(ctx, elemPath, activation);
    if (ctx.shouldReturn(status)) {
      return status;
    }
    if (itemRules_->getAnyRules() != nullptr) {
      const auto& anyMsg = message.GetReflection()->GetRepeatedMessage(message, field_, i);
      status = itemRules_->ValidateAny(ctx, elemPath, anyMsg);
      if (ctx.shouldReturn(status)) {
        return status;
      }
    }
  }
  return absl::OkStatus();
}

absl::Status MapConstraintRules::Validate(
    ConstraintContext& ctx,
    std::string_view fieldPath,
    const google::protobuf::Message& message) const {
  auto status = Base::Validate(ctx, fieldPath, message);
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
  std::string subPath =
      fieldPath.empty() ? field_->name() : absl::StrCat(fieldPath, ".", field_->name());
  for (int i = 0; i < mapVal.size(); i++) {
    const auto& elemMsg = message.GetReflection()->GetRepeatedMessage(message, field_, i);
    std::string elemPath = absl::StrCat(subPath, "[", makeMapKeyString(elemMsg, keyField), "]");
    auto key = keys[i];
    if (keyRules_ != nullptr) {
      activation.InsertValue("this", key);
      status = keyRules_->ValidateCel(ctx, elemPath, activation);
      if (ctx.shouldReturn(status)) {
        return status;
      }
      activation.RemoveValueEntry("this");
    }
    if (valueRules_ != nullptr) {
      auto value = mapVal[key];
      if (value.has_value()) {
        activation.InsertValue("this", *value);
        status = valueRules_->ValidateCel(ctx, elemPath, activation);
        if (ctx.shouldReturn(status)) {
          return status;
        }
        activation.RemoveValueEntry("this");
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
    std::string_view fieldPath,
    const google::protobuf::Message& message) const {
  std::string subPath;
  if (fieldPath.empty()) {
    fieldPath = oneof_->name();
  } else {
    subPath = absl::StrCat(fieldPath, ".", oneof_->name());
    fieldPath = subPath;
  }
  if (required_) {
    if (!message.GetReflection()->HasOneof(message, oneof_)) {
      auto& violation = *ctx.violations.add_violations();
      *violation.mutable_constraint_id() = "required";
      *violation.mutable_message() = "exactly one of oneof fields is required";
      *violation.mutable_field_path() = fieldPath;
    }
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal
