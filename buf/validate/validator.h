#pragma once

#include <memory>
#include <string_view>

#include "buf/validate/expression.pb.h"
#include "buf/validate/internal/constraints.h"
#include "buf/validate/internal/message_rules.h"
#include "eval/public/cel_expression.h"
#include "google/protobuf/message.h"

namespace buf::validate {

class ValidatorFactory;

/// A validator is a non-thread safe object that can be used to validate
/// google.protobuf.Message objects.
///
/// Validators are created through a ValidatorFactory, which is thread safe and
/// used to share cached state between validators.
class Validator {
 public:
  /// Validate a message.
  ///
  /// An empty Violations object is returned, if the message passes validation.
  /// If the message fails validation, a Violations object with the violations is returned.
  /// If there is an error while validating, a Status with the error is returned.
  absl::StatusOr<Violations> Validate(
      const google::protobuf::Message& message, std::string_view fieldPath = {});

  // Move only.
  Validator(const Validator&) = delete;
  Validator& operator=(const Validator&) = delete;
  Validator(Validator&&) = default;
  Validator& operator=(Validator&&) = default;

 private:
  friend class ValidatorFactory;

  ValidatorFactory* factory_;
  google::protobuf::Arena* arena_;
  bool failFast_;

  Validator(ValidatorFactory* factory, google::protobuf::Arena* arena, bool failFast) noexcept
      : factory_(factory), arena_(arena), failFast_(failFast) {}

  absl::Status ValidateMessage(
      internal::ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& message);

  absl::Status ValidateFields(
      internal::ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& message);
};

/// A factory that stores shared state for creating validators.
///
/// ValidatorFactory is thread-safe and can be used to create multiple
/// Validator objects, which are not thread-safe. Generally there should be
/// one factory per process, and one validator created per ~request.
class ValidatorFactory {
 public:
  /// Create a new factory.
  static absl::StatusOr<std::unique_ptr<ValidatorFactory>> New();

  /// Create a new validator using the given arena for allocations during validation.
  [[nodiscard]] Validator NewValidator(google::protobuf::Arena* arena, bool failFast = false) {
    return {this, arena, failFast};
  }

  /// Not copyable or movable.
  ValidatorFactory(const ValidatorFactory&) = delete;
  ValidatorFactory& operator=(const ValidatorFactory&) = delete;
  ValidatorFactory(ValidatorFactory&&) = delete;
  ValidatorFactory& operator=(ValidatorFactory&&) = delete;

  /// Populates the factory with constraints for the given message type.
  absl::Status Add(const google::protobuf::Descriptor* desc);

  /// Disable lazy loading of constraints.
  void DisableLazyLoading(bool disable = true) {
    absl::WriterMutexLock lock(&mutex_);
    disableLazyLoading_ = disable;
  }

 private:
  friend class Validator;
  google::protobuf::Arena arena_;
  absl::Mutex mutex_;
  absl::flat_hash_map<const google::protobuf::Descriptor*, internal::Constraints> constraints_
      ABSL_GUARDED_BY(mutex_);
  std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder> builder_
      ABSL_GUARDED_BY(mutex_);
  bool disableLazyLoading_ ABSL_GUARDED_BY(mutex_) = false;

  ValidatorFactory() = default;

  const internal::Constraints* GetMessageConstraints(const google::protobuf::Descriptor* desc);
  const internal::Constraints& AddConstraints(const google::protobuf::Descriptor* desc);
};

} // namespace buf::validate
