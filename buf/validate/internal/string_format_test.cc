// Copyright 2023 Buf Technologies, Inc.
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

#include "buf/validate/internal/string_format.h"

#include "gtest/gtest.h"

namespace buf::validate::internal {
namespace {

class StringFormatTest : public ::testing::Test {
 protected:
  StringFormat fmt_;
  std::string builder_;

  std::string build() {
    std::string result = std::move(builder_);
    builder_ = {};
    return result;
  }
};

TEST_F(StringFormatTest, Numbers) {
  fmt_.formatUnsigned(builder_, 0, 10);
  EXPECT_EQ(build(), "0");
  fmt_.formatUnsigned(builder_, 1, 10);
  EXPECT_EQ(build(), "1");
  fmt_.formatUnsigned(builder_, 9, 10);
  EXPECT_EQ(build(), "9");
  fmt_.formatUnsigned(builder_, 10, 10);
  EXPECT_EQ(build(), "10");
  fmt_.formatUnsigned(builder_, 11, 10);
  EXPECT_EQ(build(), "11");
  fmt_.formatUnsigned(builder_, 99, 10);
  EXPECT_EQ(build(), "99");
  fmt_.formatUnsigned(builder_, 100, 10);
  EXPECT_EQ(build(), "100");
  fmt_.formatUnsigned(builder_, 101, 10);
  EXPECT_EQ(build(), "101");
}

} // namespace
} // namespace buf::validate::internal