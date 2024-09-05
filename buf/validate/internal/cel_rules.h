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

#include "absl/status/status.h"
#include "buf/validate/internal/cel_constraint_rules.h"
#include "buf/validate/internal/message_factory.h"
#include "buf/validate/validate.pb.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"

namespace buf::validate::internal {

template <typename R>
absl::Status BuildCelRules(
    std::unique_ptr<MessageFactory>& messageFactory,
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const R& rules,
    CelConstraintRules& result) {
  // Look for constraints on the set fields.
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  google::protobuf::Message* reparsedRules{};
  if (messageFactory) {
    reparsedRules = messageFactory->messageFactory()
                        ->GetPrototype(messageFactory->descriptorPool()->FindMessageTypeByName(
                            rules.GetTypeName()))
                        ->New(arena);
    if (!Reparse(*messageFactory, rules, reparsedRules)) {
      reparsedRules = nullptr;
    }
  }
  if (reparsedRules) {
    result.setRules(reparsedRules, arena);
    reparsedRules->GetReflection()->ListFields(*reparsedRules, &fields);
  } else {
    result.setRules(&rules, arena);
    R::GetReflection()->ListFields(rules, &fields);
  }
  for (const auto* field : fields) {
    if (!field->options().HasExtension(buf::validate::priv::field)) {
      continue;
    }
    const auto& fieldLvl = field->options().GetExtension(buf::validate::priv::field);
    for (const auto& constraint : fieldLvl.cel()) {
      auto status = result.Add(
          builder, constraint.id(), constraint.message(), constraint.expression(), field);
      if (!status.ok()) {
        return status;
      }
    }
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal
