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

#include <memory>
#include <string_view>

#include "buf/validate/expression.pb.h"
#include "buf/validate/internal/constraints.h"
#include "buf/validate/internal/message_rules.h"
#include "buf/validate/internal/message_factory.h"
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
  absl::StatusOr<Violations> Validate(const google::protobuf::Message& message);

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
      internal::ConstraintContext& ctx, const google::protobuf::Message& message);

  absl::Status ValidateFields(
      internal::ConstraintContext& ctx, const google::protobuf::Message& message);
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

  /// Set message factory and descriptor pool. This is used for re-parsing unknown fields.
  /// The provided messageFactory and descriptorPool must outlive the ValidatorFactory.
  void SetMessageFactory(google::protobuf::MessageFactory *messageFactory,
                         const google::protobuf::DescriptorPool *descriptorPool) {
    messageFactory_ = std::make_unique<internal::MessageFactory>(messageFactory, descriptorPool);
  }

  /// Set whether or not unknown constraint fields will be tolerated. Defaults to false.
  void SetAllowUnknownFields(bool allowUnknownFields) {
    allowUnknownFields_ = allowUnknownFields;
  }

 private:
  friend class Validator;
  google::protobuf::Arena arena_;
  absl::Mutex mutex_;
  std::unique_ptr<internal::MessageFactory> messageFactory_;
  bool allowUnknownFields_;
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
