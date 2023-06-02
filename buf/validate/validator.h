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
#include "eval/public/cel_expression.h"
#include "google/protobuf/message.h"

namespace buf::validate {

class ValidatorFactory;

/// A validator is a non-thread safe object that can be used to validate
/// google.protobuf.Message objects.
class Validator {
 public:
  absl::StatusOr<Violations> Validate(
      const google::protobuf::Message& message, std::string_view fieldPath = {});

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
  static absl::StatusOr<std::unique_ptr<ValidatorFactory>> New();

  [[nodiscard]] std::unique_ptr<Validator> NewValidator(
      google::protobuf::Arena* arena, bool failFast) {
    return std::unique_ptr<Validator>(new Validator(this, arena, failFast));
  }

 private:
  friend class Validator;
  std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder> builder_;
  absl::Mutex mutex_;
  absl::flat_hash_map<const google::protobuf::Descriptor*, internal::Constraints> constraints_
      ABSL_GUARDED_BY(mutex_);
  google::protobuf::Arena arena_;

  ValidatorFactory() = default;

  const internal::Constraints& GetMessageConstraints(const google::protobuf::Descriptor* desc);
};

} // namespace buf::validate
