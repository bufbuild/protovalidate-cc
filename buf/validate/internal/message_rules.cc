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

#include "buf/validate/internal/message_rules.h"

#include "buf/validate/internal/field_rules.h"

namespace buf::validate::internal {

absl::StatusOr<std::unique_ptr<MessageConstraintRules>> BuildMessageRules(
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const MessageConstraints& constraints) {
  auto result = std::make_unique<MessageConstraintRules>();
  for (const auto& constraint : constraints.cel()) {
    if (auto status = result->Add(builder, constraint, absl::nullopt, nullptr); !status.ok()) {
      return status;
    }
  }
  return result;
}

Constraints NewMessageConstraints(
    std::unique_ptr<MessageFactory>& messageFactory,
    bool allowUnknownFields,
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::Descriptor* descriptor) {
  std::vector<std::unique_ptr<ConstraintRules>> result;
  if (descriptor->options().HasExtension(buf::validate::message)) {
    const auto& msgLvl = descriptor->options().GetExtension(buf::validate::message);
    if (msgLvl.disabled()) {
      return result;
    }
    auto rules_or = BuildMessageRules(builder, msgLvl);
    if (!rules_or.ok()) {
      return rules_or.status();
    }
    result.emplace_back(std::move(rules_or).value());
  }

  for (int i = 0; i < descriptor->field_count(); i++) {
    const google::protobuf::FieldDescriptor* field = descriptor->field(i);
    if (!field->options().HasExtension(buf::validate::field)) {
      continue;
    }
    const auto& fieldLvl = field->options().GetExtension(buf::validate::field);
    auto rules_or =
        NewFieldRules(messageFactory, allowUnknownFields, arena, builder, field, fieldLvl);
    if (!rules_or.ok()) {
      return rules_or.status();
    }
    if (rules_or.value() != nullptr) {
      result.emplace_back(std::move(rules_or).value());
    }
  }

  for (int i = 0; i < descriptor->oneof_decl_count(); i++) {
    const google::protobuf::OneofDescriptor* oneof = descriptor->oneof_decl(i);
    if (!oneof->options().HasExtension(buf::validate::oneof)) {
      continue;
    }
    const auto& oneofLvl = oneof->options().GetExtension(buf::validate::oneof);
    result.emplace_back(std::make_unique<OneofConstraintRules>(oneof, oneofLvl));
  }

  return result;
}

} // namespace buf::validate::internal
