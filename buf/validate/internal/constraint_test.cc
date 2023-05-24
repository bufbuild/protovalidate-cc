#include "buf/validate/internal/constraint.h"

#include <memory>

#include "eval/public/activation.h"
#include "eval/public/cel_expression.h"
#include "gtest/gtest.h"

namespace buf::validate::internal {
namespace cel = google::api::expr;
namespace {

class ExpressionTest : public testing::Test {
 public:
  void SetUp() override {
    cel::runtime::InterpreterOptions options;
    options.enable_qualified_type_identifiers = true;
    options.enable_timestamp_duration_overflow_errors = true;
    options.enable_heterogeneous_equality = true;
    options.enable_empty_wrapper_null_unboxing = true;
    auto builder_or = NewConstraintBuilder();
    ASSERT_TRUE(builder_or.ok());
    builder_ = std::move(builder_or).value();
  }

 protected:
  std::unique_ptr<cel::runtime::CelExpressionBuilder> builder_;
  ConstraintSet constraints_;
  google::protobuf::Arena arena_;

  absl::Status AddConstraint(std::string expr, std::string message, std::string id) {
    Constraint constraint;
    constraint.set_expression(std::move(expr));
    constraint.set_message(std::move(message));
    constraint.set_id(std::move(id));
    return constraints_.Add(*builder_, constraint);
  }

  absl::Status Validate(
      const std::string& fieldPath,
      google::api::expr::runtime::Activation& activation,
      std::vector<Violation>& violations) {
    ConstraintContext ctx;
    ctx.fieldPath = fieldPath;
    ctx.arena = &arena_;
    auto status = constraints_.Validate(ctx, activation);
    if (!status.ok()) {
      return status;
    }
    for (const auto& violation : ctx.violations.violations()) {
      violations.push_back(violation);
    }
    return absl::OkStatus();
  }
};

TEST_F(ExpressionTest, BoolResult) {
  ASSERT_TRUE(AddConstraint("true", "always succeeds", "always-succeeds").ok());
  ASSERT_TRUE(AddConstraint("false", "always fails", "always-fails").ok());

  cel::runtime::Activation ctx;
  std::vector<Violation> violations;
  auto status = Validate("a.b", ctx, violations);
  ASSERT_TRUE(status.ok()) << status;
  ASSERT_EQ(violations.size(), 1);
  EXPECT_EQ(violations[0].field_path(), "a.b");
  EXPECT_EQ(violations[0].message(), "always fails");
  EXPECT_EQ(violations[0].constraint_id(), "always-fails");
}

TEST_F(ExpressionTest, StringResult) {
  ASSERT_TRUE(AddConstraint("''", "always succeeds", "always-succeeds").ok());
  ASSERT_TRUE(AddConstraint("'error'", "always fails", "always-fails").ok());

  cel::runtime::Activation ctx;
  std::vector<Violation> violations;
  auto status = Validate("a.b", ctx, violations);
  ASSERT_TRUE(status.ok()) << status;
  ASSERT_EQ(violations.size(), 1);
  EXPECT_EQ(violations[0].field_path(), "a.b");
  EXPECT_EQ(violations[0].message(), "error");
  EXPECT_EQ(violations[0].constraint_id(), "always-fails");
}

TEST_F(ExpressionTest, Error) {
  ASSERT_TRUE(AddConstraint("1/0", "always fails", "always-fails").ok());
  cel::runtime::Activation ctx;
  std::vector<Violation> violations;
  auto status = Validate("a.b", ctx, violations);
  ASSERT_FALSE(status.ok()) << status;
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(status.message(), "divide by zero");
}

TEST_F(ExpressionTest, BadType) {
  ASSERT_TRUE(AddConstraint("1", "always fails", "always-fails").ok());
  cel::runtime::Activation ctx;
  std::vector<Violation> violations;
  auto status = Validate("a.b", ctx, violations);
  ASSERT_FALSE(status.ok()) << status;
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(status.message(), "invalid result type");
}

} // namespace
} // namespace buf::validate::internal
