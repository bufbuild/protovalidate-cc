#include "buf/validate/conformance/runner.h"

#include "buf/validate/conformance/cases/numbers.pb.h"
#include "gtest/gtest.h"

namespace buf::validate::conformance {
namespace {

TEST(ConformanceRunnerTest, Test) {
  TestRunner runner;
  cases::Int32GT int32_gt;
  int32_gt.set_val(1);
  auto result = runner.runTestCase(int32_gt);
  // EXPECT_TRUE(result.success()) << result.DebugString();

  google::protobuf::Any any;
  any.PackFrom(int32_gt);
  result = runner.runTestCase(int32_gt.descriptor(), any);
  // EXPECT_TRUE(result.success()) << result.DebugString();
}

} // namespace
} // namespace buf::validate::conformance