#pragma once

#include "absl/status/status.h"
#include "buf/validate/internal/cel_constraint_rules.h"
#include "buf/validate/validate.pb.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"

namespace buf::validate::internal {

template <typename R>
absl::Status BuildCelRules(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const R& rules,
    CelConstraintRules& result) {
  result.setRules(&rules, arena);
  // Look for constraints on the set fields.
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  R::GetReflection()->ListFields(rules, &fields);
  for (const auto* field : fields) {
    if (!field->options().HasExtension(buf::validate::priv::field)) {
      continue;
    }
    const auto& fieldLvl = field->options().GetExtension(buf::validate::priv::field);
    for (const auto& constraint : fieldLvl.cel()) {
      auto status =
          result.Add(builder, constraint.id(), constraint.message(), constraint.expression());
      if (!status.ok()) {
        return status;
      }
    }
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal