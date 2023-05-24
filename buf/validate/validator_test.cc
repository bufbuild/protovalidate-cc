#include "buf/validate/validator.h"

#include "buf/validate/conformance/cases/bool.pb.h"
#include "buf/validate/conformance/cases/custom_constraints/custom_constraints.pb.h"
#include "eval/public/activation.h"
#include "eval/public/builtin_func_registrar.h"
#include "eval/public/cel_expr_builder_factory.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "parser/parser.h"

namespace buf::validate {
namespace cel = google::api::expr;
namespace {

TEST(ValidatorTest, ParseAndEval) {
  std::string input = "1 + 2";
  auto pexpr_or = cel::parser::Parse(input);
  EXPECT_TRUE(pexpr_or.ok());
  cel::v1alpha1::ParsedExpr pexpr = std::move(pexpr_or).value();
  cel::runtime::Activation activation;

  cel::runtime::InterpreterOptions options;
  options.enable_qualified_type_identifiers = true;
  options.enable_timestamp_duration_overflow_errors = true;
  options.enable_heterogeneous_equality = true;
  options.enable_empty_wrapper_null_unboxing = true;
  std::unique_ptr<cel::runtime::CelExpressionBuilder> builder =
      cel::runtime::CreateCelExpressionBuilder(options);
  auto register_status = cel::runtime::RegisterBuiltinFunctions(builder->GetRegistry(), options);
  EXPECT_TRUE(register_status.ok());

  auto expr_or = builder->CreateExpression(pexpr.mutable_expr(), pexpr.mutable_source_info());
  EXPECT_TRUE(expr_or.ok());
  std::unique_ptr<cel::runtime::CelExpression> cel_expr = std::move(expr_or).value();
  google::protobuf::Arena arena;
  auto result_or = cel_expr->Evaluate(activation, &arena);
  EXPECT_TRUE(result_or.ok());
  cel::runtime::CelValue result = std::move(result_or).value();
  EXPECT_EQ(result.Int64OrDie(), 3);
}

TEST(ValidatorTest, ValidateBool) {
  conformance::cases::BoolConstFalse bool_const_false;
  bool_const_false.set_val(true);
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(bool_const_false);
  EXPECT_TRUE(violations_or.ok()) << violations_or.status();
  // TODO(afuller): This should report a violation.
  EXPECT_EQ(violations_or.value().violations_size(), 0);
}

TEST(ValidatorTest, MessageConstraint) {
  conformance::cases::custom_constraints::MessageExpressions message_expressions;
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(message_expressions);
  EXPECT_TRUE(violations_or.ok()) << violations_or.status();
  EXPECT_EQ(violations_or.value().violations_size(), 2);
  EXPECT_EQ(violations_or.value().violations(0).field_path(), "");
  EXPECT_EQ(violations_or.value().violations(0).constraint_id(), "message_expression_scalar");
  EXPECT_EQ(violations_or.value().violations(0).message(), "a must be less than b");
  EXPECT_EQ(violations_or.value().violations(1).field_path(), "");
  EXPECT_EQ(violations_or.value().violations(1).constraint_id(), "message_expression_enum");
  EXPECT_EQ(violations_or.value().violations(1).message(), "c must not equal d");
}

} // namespace
} // namespace buf::validate
