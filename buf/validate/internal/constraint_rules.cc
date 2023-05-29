#include "buf/validate/internal/constraint_rules.h"

#include "eval/public/structs/cel_proto_wrapper.h"

namespace buf::validate::internal {
namespace cel = google::api::expr;

void ConstraintRules::setRules(
    const google::protobuf::Message* rules, google::protobuf::Arena* arena) {
  rules_ = cel::runtime::CelProtoWrapper::CreateMessage(rules, arena);
}

} // namespace buf::validate::internal
