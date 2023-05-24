#include "buf/validate/internal/constraint.h"

#include "absl/status/statusor.h"
#include "eval/public/builtin_func_registrar.h"
#include "eval/public/cel_expr_builder_factory.h"
#include "eval/public/cel_value.h"
#include "parser/parser.h"

namespace buf::validate::internal {
namespace cel = google::api::expr;

absl::Status
ConstraintSet::Add(google::api::expr::runtime::CelExpressionBuilder &builder,
                   Constraint constraint) {
  auto pexpr_or = cel::parser::Parse(constraint.expression());
  if (!pexpr_or.ok()) {
    return pexpr_or.status();
  }
  cel::v1alpha1::ParsedExpr pexpr = std::move(pexpr_or).value();
  auto expr_or = builder.CreateExpression(pexpr.mutable_expr(),
                                          pexpr.mutable_source_info());
  if (!expr_or.ok()) {
    return expr_or.status();
  }
  std::unique_ptr<cel::runtime::CelExpression> expr =
      std::move(expr_or).value();
  exprs_.emplace_back(
      CompiledConstraint{std::move(constraint), std::move(expr)});
  return absl::OkStatus();
}

absl::Status
ConstraintSet::Validate(const google::api::expr::runtime::BaseActivation &ctx,
                        const std::string &fieldPath,
                        google::protobuf::Arena *arena,
                        std::vector<Violation> &violations) {
  for (const auto &expr : exprs_) {
    auto result_or = expr.expr->Evaluate(ctx, arena);
    if (!result_or.ok()) {
      return result_or.status();
    }
    cel::runtime::CelValue result = std::move(result_or).value();
    if (result.IsBool()) {
      if (!result.BoolOrDie()) {
        // Add violation with the constraint message.
        Violation violation;
        violation.set_field_path(fieldPath);
        violation.set_message(expr.constraint.message());
        violation.set_constraint_id(expr.constraint.id());
        violations.push_back(std::move(violation));
      }
    } else if (result.IsString()) {
      if (!result.StringOrDie().value().empty()) {
        // Add violation with custom message.
        Violation violation;
        violation.set_field_path(fieldPath);
        violation.set_message(std::string(result.StringOrDie().value()));
        violation.set_constraint_id(expr.constraint.id());
        violations.push_back(std::move(violation));
      }
    } else if (result.IsError()) {
      const cel::runtime::CelError &error = *result.ErrorOrDie();
      return {absl::StatusCode::kInvalidArgument, error.message()};
    } else {
      return {absl::StatusCode::kInvalidArgument, "invalid result type"};
    }
  }
  return absl::OkStatus();
}

absl::StatusOr<
    std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder>>
NewConstraintBuilder() {
  cel::runtime::InterpreterOptions options;
  options.enable_qualified_type_identifiers = true;
  options.enable_timestamp_duration_overflow_errors = true;
  options.enable_heterogeneous_equality = true;
  options.enable_empty_wrapper_null_unboxing = true;
  std::unique_ptr<cel::runtime::CelExpressionBuilder> builder =
      cel::runtime::CreateCelExpressionBuilder(options);
  auto register_status =
      cel::runtime::RegisterBuiltinFunctions(builder->GetRegistry(), options);
  if (!register_status.ok()) {
    return register_status;
  }
  return builder;
}

MessageConstraints
NewMessageConstraints(google::api::expr::runtime::CelExpressionBuilder &builder,
                      const google::protobuf::Descriptor *descriptor) {
  std::vector<MessageConstraint> result;
  // TODO: Add any message level constraints.
  for (int i = 0; i < descriptor->field_count(); i++) {
    const google::protobuf::FieldDescriptor *field = descriptor->field(i);
    // TODO: Add any field level constraints.
  }
  return result;
}

} // namespace buf::validate::internal
