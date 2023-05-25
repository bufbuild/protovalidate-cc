#include "buf/validate/internal/constraint.h"

#include "absl/status/statusor.h"
#include "buf/validate/internal/extra_func.h"
#include "buf/validate/priv/private.pb.h"
#include "buf/validate/validate.pb.h"
#include "eval/public/builtin_func_registrar.h"
#include "eval/public/cel_expr_builder_factory.h"
#include "eval/public/cel_value.h"
#include "eval/public/containers/field_access.h"
#include "eval/public/containers/field_backed_list_impl.h"
#include "eval/public/containers/field_backed_map_impl.h"
#include "eval/public/structs/cel_proto_wrapper.h"
#include "google/protobuf/descriptor.pb.h"
#include "parser/parser.h"

namespace buf::validate::internal {
namespace cel = google::api::expr;

namespace {

absl::Status ProcessConstraint(
    ConstraintContext& ctx,
    absl::string_view fieldPath,
    const google::api::expr::runtime::BaseActivation& activation,
    const CompiledConstraint& expr) {
  auto result_or = expr.expr->Evaluate(activation, ctx.arena);
  if (!result_or.ok()) {
    return result_or.status();
  }
  cel::runtime::CelValue result = std::move(result_or).value();
  if (result.IsBool()) {
    if (!result.BoolOrDie()) {
      // Add violation with the constraint message.
      Violation& violation = *ctx.violations.add_violations();
      *violation.mutable_field_path() = fieldPath;
      violation.set_message(expr.constraint.message());
      violation.set_constraint_id(expr.constraint.id());
    }
  } else if (result.IsString()) {
    if (!result.StringOrDie().value().empty()) {
      // Add violation with custom message.
      Violation& violation = *ctx.violations.add_violations();
      *violation.mutable_field_path() = fieldPath;
      violation.set_message(std::string(result.StringOrDie().value()));
      violation.set_constraint_id(expr.constraint.id());
    }
  } else if (result.IsError()) {
    const cel::runtime::CelError& error = *result.ErrorOrDie();
    return {absl::StatusCode::kInvalidArgument, error.message()};
  } else {
    return {absl::StatusCode::kInvalidArgument, "invalid result type"};
  }
  return absl::OkStatus();
}

} // namespace

absl::Status ConstraintSet::Add(
    google::api::expr::runtime::CelExpressionBuilder& builder, Constraint constraint) {
  auto pexpr_or = cel::parser::Parse(constraint.expression());
  if (!pexpr_or.ok()) {
    return pexpr_or.status();
  }
  cel::v1alpha1::ParsedExpr pexpr = std::move(pexpr_or).value();
  auto expr_or = builder.CreateExpression(pexpr.mutable_expr(), pexpr.mutable_source_info());
  if (!expr_or.ok()) {
    return expr_or.status();
  }
  std::unique_ptr<cel::runtime::CelExpression> expr = std::move(expr_or).value();
  exprs_.emplace_back(CompiledConstraint{std::move(constraint), std::move(expr)});
  return absl::OkStatus();
}

absl::Status ConstraintSet::Add(
    google::api::expr::runtime::CelExpressionBuilder& builder,
    std::string_view id,
    std::string_view message,
    std::string_view expression) {
  Constraint constraint;
  *constraint.mutable_id() = id;
  *constraint.mutable_message() = message;
  *constraint.mutable_expression() = expression;
  return Add(builder, constraint);
}

absl::Status ConstraintSet::Validate(
    ConstraintContext& ctx,
    std::string_view fieldPath,
    google::api::expr::runtime::Activation& activation) const {
  activation.InsertValue("rules", rules_);
  absl::Status status = absl::OkStatus();
  for (const auto& expr : exprs_) {
    status = ProcessConstraint(ctx, fieldPath, activation, expr);
    if (ctx.shouldReturn(status)) {
      break;
    }
  }
  activation.RemoveValueEntry("rules");
  return status;
}

absl::StatusOr<std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder>>
NewConstraintBuilder(google::protobuf::Arena* arena) {
  cel::runtime::InterpreterOptions options;
  options.enable_qualified_type_identifiers = true;
  options.enable_timestamp_duration_overflow_errors = true;
  options.enable_heterogeneous_equality = true;
  options.enable_empty_wrapper_null_unboxing = true;
  options.enable_regex_precompilation = true;
  options.constant_folding = true;
  options.constant_arena = arena;

  std::unique_ptr<cel::runtime::CelExpressionBuilder> builder =
      cel::runtime::CreateCelExpressionBuilder(options);
  auto register_status = cel::runtime::RegisterBuiltinFunctions(builder->GetRegistry(), options);
  if (!register_status.ok()) {
    return register_status;
  }
  register_status = RegisterExtraFuncs(*builder->GetRegistry());
  if (!register_status.ok()) {
    return register_status;
  }
  return builder;
}

void ConstraintSet::setRules(
    const google::protobuf::Message* rules, google::protobuf::Arena* arena) {
  rules_ = cel::runtime::CelProtoWrapper::CreateMessage(rules, arena);
}

absl::Status BuildMessageConstraintSet(
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const MessageConstraints& constraints,
    ConstraintSet& result) {
  for (const auto& constraint : constraints.cel()) {
    if (auto status = result.Add(builder, constraint); !status.ok()) {
      return status;
    }
  }
  return absl::OkStatus();
}

template <typename R>
absl::Status BuildConstraintSet(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const R& rules,
    ConstraintSet& result) {
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
      if (auto status =
              result.Add(builder, constraint.id(), constraint.message(), constraint.expression());
          !status.ok()) {
        return status;
      }
    }
  }
  return absl::OkStatus();
}

Constraints NewMessageConstraints(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::Descriptor* descriptor) {
  std::vector<ConstraintSet> result;
  if (descriptor->options().HasExtension(buf::validate::message)) {
    const auto& msgLvl = descriptor->options().GetExtension(buf::validate::message);
    if (msgLvl.disabled()) {
      return result;
    }
    if (auto status = BuildMessageConstraintSet(builder, msgLvl, result.emplace_back(nullptr));
        !status.ok()) {
      return status;
    }
  }

  for (int i = 0; i < descriptor->field_count(); i++) {
    const google::protobuf::FieldDescriptor* field = descriptor->field(i);
    if (!field->options().HasExtension(buf::validate::field)) {
      continue;
    }
    const auto& fieldLvl = field->options().GetExtension(buf::validate::field);
    absl::Status status = absl::UnimplementedError("Not implemented");
    switch (fieldLvl.type_case()) {
      case FieldConstraints::kBool:
        status = BuildConstraintSet(arena, builder, fieldLvl.bool_(), result.emplace_back(field));
        break;
      case FieldConstraints::kFloat:
        status = BuildConstraintSet(arena, builder, fieldLvl.float_(), result.emplace_back(field));
        break;
      case FieldConstraints::kDouble:
        status = BuildConstraintSet(arena, builder, fieldLvl.double_(), result.emplace_back(field));
        break;
      case FieldConstraints::kInt32:
        status = BuildConstraintSet(arena, builder, fieldLvl.int32(), result.emplace_back(field));
        break;
      case FieldConstraints::kInt64:
        status = BuildConstraintSet(arena, builder, fieldLvl.int64(), result.emplace_back(field));
        break;
      case FieldConstraints::kUint32:
        status = BuildConstraintSet(arena, builder, fieldLvl.uint32(), result.emplace_back(field));
        break;
      case FieldConstraints::kUint64:
        status = BuildConstraintSet(arena, builder, fieldLvl.uint64(), result.emplace_back(field));
        break;
      case FieldConstraints::kSint32:
        status = BuildConstraintSet(arena, builder, fieldLvl.sint32(), result.emplace_back(field));
        break;
      case FieldConstraints::kSint64:
        status = BuildConstraintSet(arena, builder, fieldLvl.sint64(), result.emplace_back(field));
        break;
      case FieldConstraints::kFixed32:
        status = BuildConstraintSet(arena, builder, fieldLvl.fixed32(), result.emplace_back(field));
        break;
      case FieldConstraints::kFixed64:
        status = BuildConstraintSet(arena, builder, fieldLvl.fixed64(), result.emplace_back(field));
        break;
      case FieldConstraints::kSfixed32:
        status =
            BuildConstraintSet(arena, builder, fieldLvl.sfixed32(), result.emplace_back(field));
        break;
      case FieldConstraints::kSfixed64:
        status =
            BuildConstraintSet(arena, builder, fieldLvl.sfixed64(), result.emplace_back(field));
        break;
      case FieldConstraints::kString:
        status = BuildConstraintSet(arena, builder, fieldLvl.string(), result.emplace_back(field));
        break;
      case FieldConstraints::kBytes:
        status = BuildConstraintSet(arena, builder, fieldLvl.bytes(), result.emplace_back(field));
        break;
      case FieldConstraints::kEnum:
        status = BuildConstraintSet(arena, builder, fieldLvl.enum_(), result.emplace_back(field));
        break;
      case FieldConstraints::kRepeated:
        status =
            BuildConstraintSet(arena, builder, fieldLvl.repeated(), result.emplace_back(field));
        break;
      case FieldConstraints::kMap:
        status = BuildConstraintSet(arena, builder, fieldLvl.map(), result.emplace_back(field));
        break;
      case FieldConstraints::kAny:
        status = BuildConstraintSet(arena, builder, fieldLvl.any(), result.emplace_back(field));
        break;
      case FieldConstraints::kDuration:
        status =
            BuildConstraintSet(arena, builder, fieldLvl.duration(), result.emplace_back(field));
        break;
      case FieldConstraints::kTimestamp:
        status =
            BuildConstraintSet(arena, builder, fieldLvl.timestamp(), result.emplace_back(field));
        break;
      default:
        return absl::InvalidArgumentError("unknown field validator type");
    }
    if (!status.ok()) {
      return status;
    }
  }

  for (int i = 0; i < descriptor->oneof_decl_count(); i++) {
    const google::protobuf::OneofDescriptor* oneof = descriptor->oneof_decl(i);
    if (!oneof->options().HasExtension(buf::validate::oneof)) {
      continue;
    }
    const auto& oneofLvl = oneof->options().GetExtension(buf::validate::oneof);
    // TODO(afuller): Apply oneof level constraints.
    // fmt::println("Oneof level constraints: {}", oneofLvl.ShortDebugString());
  }

  return result;
}

absl::Status ConstraintSet::Apply(
    ConstraintContext& ctx,
    std::string_view fieldPath,
    const google::protobuf::Message& message) const {
  google::api::expr::runtime::Activation activation;
  cel::runtime::CelValue result;
  std::string subPath;
  if (field_ != nullptr) {
    if (field_->is_map()) {
      result = cel::runtime::CelValue::CreateMap(
          google::protobuf::Arena::Create<cel::runtime::FieldBackedMapImpl>(
              ctx.arena, &message, field_, ctx.arena));
    } else if (field_->is_repeated()) {
      result = cel::runtime::CelValue::CreateList(
          google::protobuf::Arena::Create<cel::runtime::FieldBackedListImpl>(
              ctx.arena, &message, field_, ctx.arena));
    } else if (auto status =
                   cel::runtime::CreateValueFromSingleField(&message, field_, ctx.arena, &result);
               !status.ok()) {
      return status;
    }
    activation.InsertValue("this", result);
    if (fieldPath.empty()) {
      fieldPath = field_->name();
    } else {
      subPath = absl::StrCat(fieldPath, ".", field_->name());
      fieldPath = subPath;
    }
  } else {
    activation.InsertValue(
        "this", cel::runtime::CelProtoWrapper::CreateMessage(&message, ctx.arena));
  }
  return Validate(ctx, fieldPath, activation);
}

} // namespace buf::validate::internal
