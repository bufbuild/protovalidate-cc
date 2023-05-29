#pragma once

#include "absl/status/status.h"
#include "buf/validate/expression.pb.h"
#include "eval/public/cel_value.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message.h"

namespace buf::validate::internal {
struct ConstraintContext {
  ConstraintContext() : failFast(false), arena(nullptr) {}
  ConstraintContext(const ConstraintContext&) = delete;
  void operator=(const ConstraintContext&) = delete;

  bool failFast;
  google::protobuf::Arena* arena;
  Violations violations;

  [[nodiscard]] bool shouldReturn(const absl::Status status) {
    return !status.ok() || (failFast && violations.violations_size() > 0);
  }
};

class ConstraintRules {
 public:
  ConstraintRules() = default;
  virtual ~ConstraintRules() = default;

  ConstraintRules(const ConstraintRules&) = delete;
  void operator=(const ConstraintRules&) = delete;

  virtual absl::Status Validate(
      ConstraintContext& ctx,
      std::string_view fieldPath,
      const google::protobuf::Message& message) const = 0;
};

} // namespace buf::validate::internal
