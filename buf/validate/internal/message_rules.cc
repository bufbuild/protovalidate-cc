#include "buf/validate/internal/message_rules.h"

#include "buf/validate/internal/field_rules.h"

namespace buf::validate::internal {

absl::StatusOr<std::unique_ptr<MessageConstraintRules>> BuildMessageRules(
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const MessageConstraints& constraints) {
  auto result = std::make_unique<MessageConstraintRules>();
  for (const auto& constraint : constraints.cel()) {
    if (auto status = result->Add(builder, constraint); !status.ok()) {
      return status;
    }
  }
  return result;
}

Constraints NewMessageConstraints(
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
    auto rules_or = BuildFieldRules(arena, builder, field, fieldLvl);
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