#include "buf/validate/validator.h"

#include "buf/validate/conformance/cases/bool.pb.h"
#include "buf/validate/conformance/cases/bytes.pb.h"
#include "buf/validate/conformance/cases/custom_constraints/custom_constraints.pb.h"
#include "buf/validate/conformance/cases/strings.pb.h"
#include "eval/public/activation.h"
#include "eval/public/builtin_func_registrar.h"
#include "eval/public/cel_expr_builder_factory.h"
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
  ASSERT_TRUE(violations_or.ok()) << violations_or.status();
  ASSERT_EQ(violations_or.value().violations_size(), 1);
  EXPECT_EQ(violations_or.value().violations(0).field_path(), "val");
  EXPECT_EQ(violations_or.value().violations(0).constraint_id(), "bool.const");
  EXPECT_EQ(violations_or.value().violations(0).message(), "value must equal false");
}

TEST(ValidatorTest, ValidateStringContainsFailure) {
  conformance::cases::StringContains str_contains;
  str_contains.set_val("somethingwithout");
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(str_contains);
  ASSERT_TRUE(violations_or.ok()) << violations_or.status();
  EXPECT_EQ(violations_or.value().violations_size(), 1);
  EXPECT_EQ(violations_or.value().violations(0).field_path(), "val");
  EXPECT_EQ(violations_or.value().violations(0).constraint_id(), "string.contains");
  EXPECT_EQ(
      violations_or.value().violations(0).message(), "value does not contain substring `bar`");
}

TEST(ValidatorTest, ValidateStringContainsSuccess) {
  conformance::cases::StringContains str_contains;
  str_contains.set_val("foobar");
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(str_contains);
  ASSERT_TRUE(violations_or.ok()) << violations_or.status();
  EXPECT_EQ(violations_or.value().violations_size(), 0);
}

TEST(ValidatorTest, ValidateBytesContainsFailure) {
  conformance::cases::BytesContains bytes_contains;
  bytes_contains.set_val("somethingwithout");
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(bytes_contains);
  ASSERT_TRUE(violations_or.ok()) << violations_or.status();
  EXPECT_EQ(violations_or.value().violations_size(), 1);
  EXPECT_EQ(violations_or.value().violations(0).field_path(), "val");
  EXPECT_EQ(violations_or.value().violations(0).constraint_id(), "bytes.contains");
  EXPECT_EQ(violations_or.value().violations(0).message(), "value does not contain 626172");
}

TEST(ValidatorTest, ValidateBytesContainsSuccess) {
  conformance::cases::BytesContains bytes_contains;
  bytes_contains.set_val("foobar");
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(bytes_contains);
  ASSERT_TRUE(violations_or.ok()) << violations_or.status();
  EXPECT_EQ(violations_or.value().violations_size(), 0);
}

TEST(ValidatorTest, ValidateStartsWithFailure) {
  conformance::cases::StringPrefix str_starts_with;
  str_starts_with.set_val("ffoobar");
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(str_starts_with);
  ASSERT_TRUE(violations_or.ok()) << violations_or.status();
  EXPECT_EQ(violations_or.value().violations_size(), 1);
  EXPECT_EQ(violations_or.value().violations(0).field_path(), "val");
  EXPECT_EQ(violations_or.value().violations(0).constraint_id(), "string.prefix");
  EXPECT_EQ(violations_or.value().violations(0).message(), "value does not have prefix `foo`");
}

TEST(ValidatorTest, ValidateStartsWithSuccess) {
  conformance::cases::StringPrefix str_starts_with;
  str_starts_with.set_val("foobar");
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(str_starts_with);
  ASSERT_TRUE(violations_or.ok()) << violations_or.status();
  EXPECT_EQ(violations_or.value().violations_size(), 0);
}

TEST(ValidatorTest, ValidateEndsWithFailure) {
  conformance::cases::StringSuffix str_ends_with;
  str_ends_with.set_val("foobazz");
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(str_ends_with);
  ASSERT_TRUE(violations_or.ok()) << violations_or.status();
  EXPECT_EQ(violations_or.value().violations_size(), 1);
  EXPECT_EQ(violations_or.value().violations(0).field_path(), "val");
  EXPECT_EQ(violations_or.value().violations(0).constraint_id(), "string.suffix");
  EXPECT_EQ(violations_or.value().violations(0).message(), "value does not have suffix `baz`");
}

TEST(ValidatorTest, ValidateHostnameFailure) {
  conformance::cases::StringHostname str_hostname;
  str_hostname.set_val("@!#$%^&*&^%$#");
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(str_hostname);
  ASSERT_TRUE(violations_or.ok()) << violations_or.status();
  EXPECT_EQ(violations_or.value().violations_size(), 1);
  EXPECT_EQ(violations_or.value().violations(0).field_path(), "val");
  EXPECT_EQ(violations_or.value().violations(0).constraint_id(), "string.hostname");
}

TEST(ValidatorTest, ValidateEndsWithSuccess) {
  conformance::cases::StringSuffix str_ends_with;
  str_ends_with.set_val("foobaz");
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(str_ends_with);
  ASSERT_TRUE(violations_or.ok()) << violations_or.status();
  EXPECT_EQ(violations_or.value().violations_size(), 0);
}

TEST(ValidatorTest, MessageConstraint) {
  conformance::cases::custom_constraints::MessageExpressions message_expressions;
  message_expressions.mutable_e();
  auto factory_or = ValidatorFactory::New();
  ASSERT_TRUE(factory_or.ok()) << factory_or.status();
  auto factory = std::move(factory_or).value();
  google::protobuf::Arena arena;
  auto validator = factory->NewValidator(&arena, false);
  auto violations_or = validator->Validate(message_expressions);
  ASSERT_TRUE(violations_or.ok()) << violations_or.status();
  ASSERT_EQ(violations_or.value().violations_size(), 3);
  EXPECT_EQ(violations_or.value().violations(0).field_path(), "");
  EXPECT_EQ(violations_or.value().violations(0).constraint_id(), "message_expression_scalar");
  EXPECT_EQ(violations_or.value().violations(0).message(), "a must be less than b");
  EXPECT_EQ(violations_or.value().violations(1).field_path(), "");
  EXPECT_EQ(violations_or.value().violations(1).constraint_id(), "message_expression_enum");
  EXPECT_EQ(violations_or.value().violations(1).message(), "c must not equal d");
  EXPECT_EQ(violations_or.value().violations(2).field_path(), "e");
  EXPECT_EQ(violations_or.value().violations(2).constraint_id(), "message_expression_nested");
  EXPECT_EQ(violations_or.value().violations(2).message(), "a must be greater than b");
}

} // namespace
} // namespace buf::validate
