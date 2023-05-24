#include "buf/validate/validator.h"

#include "buf/validate/internal/constraint.h"

namespace buf::validate {

absl::Status Validator::ValidateImpl(
    internal::ConstraintContext& ctx,
    std::string_view fieldPath,
    const google::protobuf::Message& message) {
  const auto& constraints_or = factory_->GetMessageConstraints(message.GetDescriptor());
  if (!constraints_or.ok()) {
    return constraints_or.status();
  }
  for (const auto& constraint : constraints_or.value()) {
    auto status = constraint(ctx, fieldPath, message);
    if (!status.ok() || (failFast_ && ctx.violations.violations_size() > 0)) {
      return status;
    }
  }

  // Validate any set fields that are messages.
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  message.GetReflection()->ListFields(message, &fields);
  std::string subFieldPath;
  for (const auto& field : fields) {
    if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      continue;
    }

    std::string subFieldPath;
    if (fieldPath.empty()) {
      subFieldPath = field->name();
    } else {
      subFieldPath = absl::StrCat(fieldPath, ".", field->name());
    }
    if (field->is_repeated()) {
      int size = message.GetReflection()->FieldSize(message, field);
      for (int i = 0; i < size; i++) {
        const auto& sub_message = message.GetReflection()->GetRepeatedMessage(message, field, i);
        std::string elemPath = absl::StrCat(subFieldPath, ".", i);
        auto status = ValidateImpl(ctx, elemPath, sub_message);
        if (!status.ok() || (failFast_ && ctx.violations.violations_size() > 0)) {
          return status;
        }
      }
    } else {
      const auto& sub_message = message.GetReflection()->GetMessage(message, field);
      auto status = ValidateImpl(ctx, subFieldPath, sub_message);
      if (!status.ok() || (failFast_ && ctx.violations.violations_size() > 0)) {
        return status;
      }
    }
  }
  return absl::OkStatus();
}

absl::StatusOr<Violations> Validator::Validate(
    const google::protobuf::Message& message, std::string_view fieldPath) {
  internal::ConstraintContext ctx;
  ctx.failFast = failFast_;
  ctx.arena = arena_;
  auto status = ValidateImpl(ctx, fieldPath, message);
  if (!status.ok()) {
    return status;
  }
  return std::move(ctx.violations);
}

absl::StatusOr<std::unique_ptr<ValidatorFactory>> ValidatorFactory::New() {
  auto builder_or = internal::NewConstraintBuilder();
  if (!builder_or.ok()) {
    return builder_or.status();
  }
  return std::unique_ptr<ValidatorFactory>(new ValidatorFactory(std::move(builder_or).value()));
}

const internal::MessageConstraints& ValidatorFactory::GetMessageConstraints(
    const google::protobuf::Descriptor* desc) {
  {
    absl::ReaderMutexLock lock(&mutex_);
    auto iter = constraints_.find(desc);
    if (iter != constraints_.end()) {
      return iter->second;
    }
  }
  absl::WriterMutexLock lock(&mutex_);
  auto iter = constraints_.find(desc);
  if (iter != constraints_.end()) {
    return iter->second;
  }
  auto itr = constraints_.emplace(desc, internal::NewMessageConstraints(*builder_, desc)).first;
  return itr->second;
}

} // namespace buf::validate
