#include "buf/validate/internal/constraint.h"

#include "absl/status/statusor.h"
#include "buf/validate/validate.pb.h"
#include "eval/public/builtin_func_registrar.h"
#include "eval/public/cel_expr_builder_factory.h"
#include "eval/public/cel_value.h"
#include "eval/public/structs/cel_proto_wrapper.h"
#include "fmt/core.h"
#include "google/protobuf/descriptor.pb.h"
#include "parser/parser.h"

namespace buf::validate::internal {
namespace cel = google::api::expr;

namespace {

absl::Status ProcessConstraint(
    ConstraintContext& ctx,
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
      *violation.mutable_field_path() = ctx.fieldPath;
      violation.set_message(expr.constraint.message());
      violation.set_constraint_id(expr.constraint.id());
    }
  } else if (result.IsString()) {
    if (!result.StringOrDie().value().empty()) {
      // Add violation with custom message.
      Violation& violation = *ctx.violations.add_violations();
      *violation.mutable_field_path() = ctx.fieldPath;
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

absl::Status ConstraintSet::Validate(
    ConstraintContext& ctx, google::api::expr::runtime::Activation& activation) {
  activation.InsertValue("rules", rules_);
  absl::Status status = absl::OkStatus();
  for (const auto& expr : exprs_) {
    status = ProcessConstraint(ctx, activation, expr);
    if (!status.ok() || (ctx.failFast && ctx.violations.violations_size() != 0)) {
      break;
    }
  }
  activation.RemoveValueEntry("rules");
  return status;
}

absl::StatusOr<std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder>>
NewConstraintBuilder() {
  cel::runtime::InterpreterOptions options;
  options.enable_qualified_type_identifiers = true;
  options.enable_timestamp_duration_overflow_errors = true;
  options.enable_heterogeneous_equality = true;
  options.enable_empty_wrapper_null_unboxing = true;
  std::unique_ptr<cel::runtime::CelExpressionBuilder> builder =
      cel::runtime::CreateCelExpressionBuilder(options);
  auto register_status = cel::runtime::RegisterBuiltinFunctions(builder->GetRegistry(), options);
  if (!register_status.ok()) {
    return register_status;
  }
  return builder;
}

MessageConstraints NewMessageConstraints(
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::Descriptor* descriptor) {
  std::vector<MessageConstraint> result;
  fmt::println("Processing constraints for message: {}", descriptor->full_name());
  if (descriptor->options().HasExtension(buf::validate::message)) {
    const auto& msgLvl = descriptor->options().GetExtension(buf::validate::message);
    fmt::println("Message level constraints: {}", msgLvl.ShortDebugString());
    if (msgLvl.disabled()) {
      return result;
    }
    // TODO: Explicitly give ownership to caller instead of using a shared_ptr.
    auto msgSet = std::make_shared<ConstraintSet>();
    for (const auto& constraint : msgLvl.cel()) {
      auto status = msgSet->Add(builder, constraint);
      if (!status.ok()) {
        return status;
      }
    };
    result.emplace_back([msgSet](ConstraintContext& ctx, const google::protobuf::Message& message) {
      google::api::expr::runtime::Activation activation;
      activation.InsertValue(
          "this", cel::runtime::CelProtoWrapper::CreateMessage(&message, ctx.arena));
      return msgSet->Validate(ctx, activation);
    });
  }

  for (int i = 0; i < descriptor->field_count(); i++) {
    const google::protobuf::FieldDescriptor* field = descriptor->field(i);
    if (!field->options().HasExtension(buf::validate::field)) {
      continue;
    }
    const auto& fieldLvl = field->options().GetExtension(buf::validate::field);
    // TODO(afuller): Add field level constraints.
    fmt::println("Field level constraints: {}", fieldLvl.ShortDebugString());
  }

  for (int i = 0; i < descriptor->oneof_decl_count(); i++) {
    const google::protobuf::OneofDescriptor* oneof = descriptor->oneof_decl(i);
    if (!oneof->options().HasExtension(buf::validate::oneof)) {
      continue;
    }
    const auto& oneofLvl = oneof->options().GetExtension(buf::validate::oneof);
    // TODO(afuller): Apply oneof level constraints.
    fmt::println("Oneof level constraints: {}", oneofLvl.ShortDebugString());
  }

  return result;
}

} // namespace buf::validate::internal
