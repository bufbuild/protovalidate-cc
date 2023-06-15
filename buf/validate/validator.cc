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

#include "buf/validate/validator.h"

namespace buf::validate {

absl::Status Validator::ValidateMessage(
    internal::ConstraintContext& ctx, const google::protobuf::Message& message) {
  const auto* constraints_or = factory_->GetMessageConstraints(message.GetDescriptor());
  if (constraints_or == nullptr) {
    return absl::NotFoundError(
        "constraints not loaded for message: " + message.GetDescriptor()->full_name());
  }
  if (!constraints_or->ok()) {
    return constraints_or->status();
  }
  for (const auto& constraint : constraints_or->value()) {
    auto status = constraint->Validate(ctx, message);
    if (ctx.shouldReturn(status)) {
      return status;
    }
  }
  return ValidateFields(ctx, message);
}

absl::Status Validator::ValidateFields(
    internal::ConstraintContext& ctx, const google::protobuf::Message& message) {
  // Validate any set fields that are messages.
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  message.GetReflection()->ListFields(message, &fields);
  for (const auto& field : fields) {
    if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      continue;
    }
    if (field->options().HasExtension(validate::field)) {
      const auto& fieldExt = field->options().GetExtension(validate::field);
      if (fieldExt.skipped() ||
          (fieldExt.has_repeated() && fieldExt.repeated().items().skipped()) ||
          (fieldExt.has_map() && fieldExt.map().values().skipped())) {
        continue;
      }
    }
    if (field->is_map()) {
      const auto* mapEntryDesc = field->message_type();
      const auto* keyField = mapEntryDesc->FindFieldByName("key");
      const auto* valueField = mapEntryDesc->FindFieldByName("value");
      if (keyField == nullptr || valueField == nullptr) {
        return absl::InternalError("map entry missing key or value field");
      }
      if (valueField->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        continue;
      }
      int size = message.GetReflection()->FieldSize(message, field);
      for (int i = 0; i < size; i++) {
        const auto& elemMsg = message.GetReflection()->GetRepeatedMessage(message, field, i);
        const auto& valueMsg = elemMsg.GetReflection()->GetMessage(elemMsg, valueField);
        size_t pos = ctx.violations.violations_size();
        auto status = ValidateMessage(ctx, valueMsg);
        if (pos < ctx.violations.violations_size()) {
          ctx.prefixFieldPath(
              absl::StrCat(field->name(), "[", internal::makeMapKeyString(elemMsg, keyField), "]"),
              pos);
        }
        if (ctx.shouldReturn(status)) {
          return status;
        }
      }
    } else if (field->is_repeated()) {
      int size = message.GetReflection()->FieldSize(message, field);
      for (int i = 0; i < size; i++) {
        size_t pos = ctx.violations.violations_size();
        const auto& subMsg = message.GetReflection()->GetRepeatedMessage(message, field, i);
        auto status = ValidateMessage(ctx, subMsg);
        if (pos < ctx.violations.violations_size()) {
          ctx.prefixFieldPath(absl::StrCat(field->name(), "[", i, "]"), pos);
        }
        if (ctx.shouldReturn(status)) {
          return status;
        }
      }
    } else {
      const auto& subMsg = message.GetReflection()->GetMessage(message, field);
      size_t pos = ctx.violations.violations_size();
      auto status = ValidateMessage(ctx, subMsg);
      if (pos < ctx.violations.violations_size()) {
        ctx.prefixFieldPath(field->name(), pos);
      }
      if (ctx.shouldReturn(status)) {
        return status;
      }
    }
  }
  return absl::OkStatus();
}

absl::StatusOr<Violations> Validator::Validate(const google::protobuf::Message& message) {
  internal::ConstraintContext ctx;
  ctx.failFast = failFast_;
  ctx.arena = arena_;
  auto status = ValidateMessage(ctx, message);
  if (!status.ok()) {
    return status;
  }
  return std::move(ctx.violations);
}

absl::StatusOr<std::unique_ptr<ValidatorFactory>> ValidatorFactory::New() {
  std::unique_ptr<ValidatorFactory> result(new ValidatorFactory());
  auto builder_or = internal::NewConstraintBuilder(&result->arena_);
  if (!builder_or.ok()) {
    return builder_or.status();
  }
  absl::MutexLock lock(&result->mutex_);
  result->builder_ = std::move(builder_or).value();
  return result;
}

absl::Status ValidatorFactory::Add(const google::protobuf::Descriptor* desc) {
  {
    absl::WriterMutexLock lock(&mutex_);
    auto iter = constraints_.find(desc);
    if (iter != constraints_.end()) {
      return iter->second.status();
    }
    auto status =
        constraints_.emplace(desc, internal::NewMessageConstraints(&arena_, *builder_, desc))
            .first->second.status();
    if (!status.ok()) {
      return status;
    }
  }

  // Add all message fields recursively.
  for (int i = 0; i < desc->field_count(); i++) {
    const google::protobuf::FieldDescriptor* field = desc->field(i);
    if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      if (auto status = Add(field->message_type()); !status.ok()) {
        return status;
      }
    }
  }
  return absl::OkStatus();
}

const internal::Constraints* ValidatorFactory::GetMessageConstraints(
    const google::protobuf::Descriptor* desc) {
  {
    absl::ReaderMutexLock lock(&mutex_);
    auto iter = constraints_.find(desc);
    if (iter != constraints_.end()) {
      return &iter->second;
    }
    if (disableLazyLoading_) {
      return nullptr;
    }
  }
  absl::WriterMutexLock lock(&mutex_);
  auto iter = constraints_.find(desc);
  if (iter != constraints_.end()) {
    return &iter->second;
  }
  return &constraints_.emplace(desc, internal::NewMessageConstraints(&arena_, *builder_, desc))
              .first->second;
}

} // namespace buf::validate
