// Copyright 2023-2025 Buf Technologies, Inc.
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

#include "buf/validate/internal/rules.h"

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
    rules_ = std::make_unique<MessageValidationRules>();
    cel::runtime::InterpreterOptions options;
    options.enable_qualified_type_identifiers = true;
    options.enable_timestamp_duration_overflow_errors = true;
    options.enable_heterogeneous_equality = true;
    options.enable_empty_wrapper_null_unboxing = true;
    auto builder_or = NewRuleBuilder(&arena_);
    ASSERT_TRUE(builder_or.ok());
    builder_ = std::move(builder_or).value();
  }

 protected:
  std::unique_ptr<cel::runtime::CelExpressionBuilder> builder_;
  std::unique_ptr<MessageValidationRules> rules_;
  google::protobuf::Arena arena_;

  absl::Status AddRule(std::string expr, std::string message, std::string id) {
    Rule rule;
    rule.set_expression(std::move(expr));
    rule.set_message(std::move(message));
    rule.set_id(std::move(id));
    return rules_->Add(*builder_, rule, absl::nullopt, nullptr);
  }

  absl::Status Validate(
      google::api::expr::runtime::Activation& activation, std::vector<RuleViolation>& violations) {
    RuleContext ctx;
    ctx.arena = &arena_;
    auto status = rules_->ValidateCel(ctx, activation);
    if (!status.ok()) {
      return status;
    }
    for (auto& violation : ctx.violations) {
      violations.push_back(violation);
    }
    return absl::OkStatus();
  }
};

TEST_F(ExpressionTest, BoolResult) {
  ASSERT_TRUE(AddRule("true", "always succeeds", "always-succeeds").ok());
  ASSERT_TRUE(AddRule("false", "always fails", "always-fails").ok());

  cel::runtime::Activation ctx;
  std::vector<RuleViolation> violations;
  auto status = Validate(ctx, violations);
  ASSERT_TRUE(status.ok()) << status;
  ASSERT_EQ(violations.size(), 1);
  EXPECT_EQ(violations[0].proto().message(), "always fails");
  EXPECT_EQ(violations[0].proto().rule_id(), "always-fails");
}

TEST_F(ExpressionTest, StringResult) {
  ASSERT_TRUE(AddRule("''", "always succeeds", "always-succeeds").ok());
  ASSERT_TRUE(AddRule("'error'", "always fails", "always-fails").ok());

  cel::runtime::Activation ctx;
  std::vector<RuleViolation> violations;
  auto status = Validate(ctx, violations);
  ASSERT_TRUE(status.ok()) << status;
  ASSERT_EQ(violations.size(), 1);
  EXPECT_EQ(violations[0].proto().message(), "error");
  EXPECT_EQ(violations[0].proto().rule_id(), "always-fails");
}

TEST_F(ExpressionTest, Error) {
  ASSERT_TRUE(AddRule("1/0", "always fails", "always-fails").ok());
  cel::runtime::Activation ctx;
  std::vector<RuleViolation> violations;
  auto status = Validate(ctx, violations);
  ASSERT_FALSE(status.ok()) << status;
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(status.message(), "divide by zero");
}

TEST_F(ExpressionTest, BadType) {
  ASSERT_TRUE(AddRule("1", "always fails", "always-fails").ok());
  cel::runtime::Activation ctx;
  std::vector<RuleViolation> violations;
  auto status = Validate(ctx, violations);
  ASSERT_FALSE(status.ok()) << status;
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(status.message(), "invalid result type");
}

} // namespace
} // namespace buf::validate::internal
