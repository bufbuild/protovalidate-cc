#include "buf/validate/validator.h"

#include "buf/validate/internal/constraint.h"

namespace buf::validate {

absl::StatusOr<Violations> Validator::Validate(const google::protobuf::Message& message) {
  const auto& constraints_or = factory_->GetMessageConstraints(message.GetDescriptor());
  if (!constraints_or.ok()) {
    return constraints_or.status();
  }
  internal::ConstraintContext ctx;
  ctx.failFast = failFast_;
  ctx.arena = arena_;
  for (const auto& constraint : constraints_or.value()) {
    auto status = constraint(ctx, message);
    if (!status.ok()) {
      return status;
    } else if (failFast_ && ctx.violations.violations_size() > 0) {
      return std::move(ctx.violations);
    }
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
