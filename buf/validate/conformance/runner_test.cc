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