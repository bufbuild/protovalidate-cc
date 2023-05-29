#pragma once

#include "absl/status/statusor.h"
#include "buf/validate/internal/constraints.h"
#include "buf/validate/validate.pb.h"
#include "eval/public/cel_expression.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"

namespace buf::validate::internal {

using Constraints = absl::StatusOr<std::vector<std::unique_ptr<ConstraintRules>>>;

Constraints NewMessageConstraints(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::Descriptor* descriptor);

absl::StatusOr<std::unique_ptr<MessageConstraintRules>> BuildMessageRules(
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const MessageConstraints& constraints);

} // namespace buf::validate::internal