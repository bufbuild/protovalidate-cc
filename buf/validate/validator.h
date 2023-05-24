#pragma once

#include <memory>

#include "buf/validate/expression.pb.h"
#include "buf/validate/internal/constraint.h"
#include "eval/public/cel_expression.h"
#include "google/protobuf/message.h"

namespace buf::validate {

class ValidatorFactory;

/// A validator is a non-thread safe object that can be used to validate
/// google.protobuf.Message objects.
class Validator {
 public:
  absl::StatusOr<Violations> Validate(const google::protobuf::Message& message);

 private:
  friend class ValidatorFactory;

  Validator(ValidatorFactory* factory, google::protobuf::Arena* arena, bool failFast) noexcept
      : factory_(factory), arena_(arena), failFast_(failFast) {}

  ValidatorFactory* factory_;
  google::protobuf::Arena* arena_;
  bool failFast_;
};

/// A factory that stores shared state for creating validators.
///
/// ValidatorFactory is thread-safe and can be used to create multiple
/// Validator objects, which are not thread-safe. Generally there should be
/// one factory per process, and one validator created per ~request.
class ValidatorFactory {
 public:
  static absl::StatusOr<std::unique_ptr<ValidatorFactory>> New();

  [[nodiscard]] std::unique_ptr<Validator> NewValidator(
      google::protobuf::Arena* arena, bool failFast) {
    return std::unique_ptr<Validator>(new Validator(this, arena, failFast));
  }

 private:
  friend class Validator;
  std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder> builder_;
  absl::Mutex mutex_;
  absl::flat_hash_map<const google::protobuf::Descriptor*, internal::MessageConstraints>
      constraints_ ABSL_GUARDED_BY(mutex_);

  explicit ValidatorFactory(
      std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder> builder)
      : builder_(std::move(builder)) {}

  const internal::MessageConstraints& GetMessageConstraints(
      const google::protobuf::Descriptor* desc);
};

} // namespace buf::validate
