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

#include "buf/validate/internal/lib/ipv4.h"

#include <algorithm>

#include "gtest/gtest.h"

namespace buf::validate::internal::lib {
namespace {

struct IPv4AddressParseTestCase {
  std::string input;
  std::optional<IPv4Address> result;
};

class IPv4AddressParseTestSuite : public ::testing::TestWithParam<IPv4AddressParseTestCase> {};

TEST_P(IPv4AddressParseTestSuite, Parse) {
  auto param = GetParam();
  EXPECT_EQ(parseIPv4Address(param.input), param.result);
}

INSTANTIATE_TEST_SUITE_P(
    IPv4ParseTest,
    IPv4AddressParseTestSuite,
    ::testing::Values(
        IPv4AddressParseTestCase{"127.0.0.1", {{0x7f000001}}},
        IPv4AddressParseTestCase{"100.100.100.100", {{0x64646464}}},
        IPv4AddressParseTestCase{"255.255.255.255", {{0xffffffff}}},
        IPv4AddressParseTestCase{"0.0.0.0", {{0}}},
        IPv4AddressParseTestCase{"0.00.0.0", std::nullopt},
        IPv4AddressParseTestCase{"255.256.255.255", std::nullopt},
        IPv4AddressParseTestCase{"1111.255.255.255", std::nullopt},
        IPv4AddressParseTestCase{"0.x.1.y", std::nullopt},
        IPv4AddressParseTestCase{"0.0.0.", std::nullopt},
        IPv4AddressParseTestCase{".0.0.0", std::nullopt},
        IPv4AddressParseTestCase{"0..0.0.0", std::nullopt},
        IPv4AddressParseTestCase{"0.0.0.0.", std::nullopt},
        IPv4AddressParseTestCase{"0.0.0.0/32", std::nullopt},
        IPv4AddressParseTestCase{"0", std::nullopt},
        IPv4AddressParseTestCase{"", std::nullopt}),
    [](const testing::TestParamInfo<IPv4AddressParseTestCase>& info) {
      std::string name = info.param.input;
      std::replace(name.begin(), name.end(), '.', '_');
      std::replace(name.begin(), name.end(), '/', '_');
      if (name.empty()) {
        return std::string{"Empty"};
      }
      return name;
    });

struct IPv4PrefixParseTestCase {
  std::string input;
  std::optional<IPv4Prefix> result;
};

class IPv4PrefixParseTestSuite : public ::testing::TestWithParam<IPv4PrefixParseTestCase> {};

TEST_P(IPv4PrefixParseTestSuite, Parse) {
  auto param = GetParam();
  EXPECT_EQ(parseIPv4Prefix(param.input), param.result);
}

INSTANTIATE_TEST_SUITE_P(
    IPv4PrefixParseTest,
    IPv4PrefixParseTestSuite,
    ::testing::Values(
        IPv4PrefixParseTestCase{"127.0.0.1/1", {{0x7f000001, 1}}},
        IPv4PrefixParseTestCase{"100.100.100.100/0", {{0x64646464, 0}}},
        IPv4PrefixParseTestCase{"255.255.255.255/32", {{0xffffffff, 32}}},
        IPv4PrefixParseTestCase{"10.0.0.0/8", {{0x0a000000, 8}}},
        IPv4PrefixParseTestCase{"1.1.1.1//1", std::nullopt},
        IPv4PrefixParseTestCase{"1.1.1.1.1", std::nullopt},
        IPv4PrefixParseTestCase{"1.1.1.1/33", std::nullopt},
        IPv4PrefixParseTestCase{"0.0.0.0", std::nullopt},
        IPv4PrefixParseTestCase{"255.255.255.255", std::nullopt},
        IPv4PrefixParseTestCase{"0.00.0.0/0", std::nullopt},
        IPv4PrefixParseTestCase{"0.0.0.0/y", std::nullopt},
        IPv4PrefixParseTestCase{"0.0.0./0", std::nullopt},
        IPv4PrefixParseTestCase{".0.0.0/0", std::nullopt},
        IPv4PrefixParseTestCase{"0..0.0.0", std::nullopt},
        IPv4PrefixParseTestCase{"0/0", std::nullopt},
        IPv4PrefixParseTestCase{"", std::nullopt}),
    [](const testing::TestParamInfo<IPv4PrefixParseTestCase>& info) {
      std::string name = info.param.input;
      std::replace(name.begin(), name.end(), '.', '_');
      std::replace(name.begin(), name.end(), '/', '_');
      if (name.empty()) {
        return std::string{"Empty"};
      }
      return name;
    });

TEST(IPv4PrefixTest, Mask) {
  auto prefix = parseIPv4Prefix("128.0.0.0/0");
  EXPECT_EQ(prefix.has_value(), true);
  EXPECT_EQ(prefix->bits, 0x80000000);
  EXPECT_EQ(prefix->prefixLength, 0);
  EXPECT_EQ(prefix->mask(), 0x00000000);
  prefix = parseIPv4Prefix("128.0.0.0/24");
  EXPECT_EQ(prefix.has_value(), true);
  EXPECT_EQ(prefix->bits, 0x80000000);
  EXPECT_EQ(prefix->prefixLength, 24);
  EXPECT_EQ(prefix->mask(), 0xffffff00);
  prefix = parseIPv4Prefix("128.0.0.0/32");
  EXPECT_EQ(prefix.has_value(), true);
  EXPECT_EQ(prefix->bits, 0x80000000);
  EXPECT_EQ(prefix->prefixLength, 32);
  EXPECT_EQ(prefix->mask(), 0xffffffff);
}

} // namespace
} // namespace buf::validate::internal::lib
