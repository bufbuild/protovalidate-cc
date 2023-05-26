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