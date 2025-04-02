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

#include "buf/validate/internal/lib/ipv6.h"

#include <algorithm>

#include "gtest/gtest.h"

namespace buf::validate::internal::lib {
namespace {

struct IPv6ParseAddressTestCase {
  std::string input;
  std::optional<IPv6Address> result;
};

class IPv6ParseTestSuite : public ::testing::TestWithParam<IPv6ParseAddressTestCase> {};

TEST_P(IPv6ParseTestSuite, Parse) {
  auto param = GetParam();
  EXPECT_EQ(parseIPv6Address(param.input), param.result);
}

INSTANTIATE_TEST_SUITE_P(
    IPv6ParseTest,
    IPv6ParseTestSuite,
    ::testing::Values(
        IPv6ParseAddressTestCase{"::", {{{}}}},
        IPv6ParseAddressTestCase{"::0", {{{}}}},
        IPv6ParseAddressTestCase{"::1", {{{1}}}},
        IPv6ParseAddressTestCase{"0:0:0:0:0:0:0:0", {{{}}}},
        IPv6ParseAddressTestCase{"0:0:0:0::0:0:0", {{{}}}},
        IPv6ParseAddressTestCase{"::0:0:0:0:0:0:0", {{{}}}},
        IPv6ParseAddressTestCase{"0:0:0:0:0:0::0", {{{}}}},
        IPv6ParseAddressTestCase{"0000:0000:0000:0000:0000:0000:0000:0000", {{{}}}},
        IPv6ParseAddressTestCase{
            "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", {{{std::bitset<128>{}.set()}}}},
        IPv6ParseAddressTestCase{"0:0:0:0:5:6:7::", {{{0x0005000600070000}}}},
        IPv6ParseAddressTestCase{"::1%% :x\x1F", {{{1}}}},
        IPv6ParseAddressTestCase{"::ffff:127.0.0.1", {{{0xffff7f000001}}}},
        IPv6ParseAddressTestCase{"::ffff:100.100.100.100", {{{0xffff64646464}}}},
        IPv6ParseAddressTestCase{"::ffff:255.255.255.255", {{{0xffffffffffff}}}},
        IPv6ParseAddressTestCase{"::ffff:0.0.0.0", {{{0xffff00000000}}}},
        IPv6ParseAddressTestCase{"::ffff:0.0.0.0%foo", {{{0xffff00000000}}}},
        IPv6ParseAddressTestCase{"0:0:0:0::0:0:0:0", std::nullopt},
        IPv6ParseAddressTestCase{"0:0:0:0:0:0:0::0", std::nullopt},
        IPv6ParseAddressTestCase{"0::0:0:0:0:0:0:0", std::nullopt},
        IPv6ParseAddressTestCase{"::0:0:0:0:0:0:0:0", std::nullopt},
        IPv6ParseAddressTestCase{"::ffff0000", std::nullopt},
        IPv6ParseAddressTestCase{"ffff0000::0", std::nullopt},
        IPv6ParseAddressTestCase{"::ffff:0.00.0.0", std::nullopt},
        IPv6ParseAddressTestCase{"::ffff:255.256.255.255", std::nullopt},
        IPv6ParseAddressTestCase{"::ffff:1111.255.255.255", std::nullopt},
        IPv6ParseAddressTestCase{"::ffff:0.x.1.y", std::nullopt},
        IPv6ParseAddressTestCase{"::ffff:0.0.0.", std::nullopt},
        IPv6ParseAddressTestCase{"::ffff:.0.0.0", std::nullopt},
        IPv6ParseAddressTestCase{"::ffff:0..0.0.0", std::nullopt},
        IPv6ParseAddressTestCase{"::ffff:0.0.0.0.", std::nullopt},
        IPv6ParseAddressTestCase{"::ffff:0.0.0.0/32", std::nullopt},
        IPv6ParseAddressTestCase{"::ffff:", std::nullopt},
        IPv6ParseAddressTestCase{":::0", std::nullopt},
        IPv6ParseAddressTestCase{":", std::nullopt},
        IPv6ParseAddressTestCase{"0", std::nullopt},
        IPv6ParseAddressTestCase{"", std::nullopt}),
    [](const testing::TestParamInfo<IPv6ParseAddressTestCase>& info) {
      std::string name = info.param.input;
      for (auto c : {':', '.', '/', '%', '\x1F', ' '}) {
        std::replace(name.begin(), name.end(), c, '_');
      }
      if (name.empty()) {
        return std::string{"Empty"};
      }
      return name;
    });

struct IPv6PrefixParseTestCase {
  std::string input;
  std::optional<IPv6Prefix> result;
};

class IPv6PrefixParseTestSuite : public ::testing::TestWithParam<IPv6PrefixParseTestCase> {};

TEST_P(IPv6PrefixParseTestSuite, Parse) {
  auto param = GetParam();
  EXPECT_EQ(parseIPv6Prefix(param.input), param.result);
}

INSTANTIATE_TEST_SUITE_P(
    IPv6PrefixParseTest,
    IPv6PrefixParseTestSuite,
    ::testing::Values(
        IPv6PrefixParseTestCase{"::/1", {{{}, 1}}},
        IPv6PrefixParseTestCase{"::0/1", {{{}, 1}}},
        IPv6PrefixParseTestCase{"::1/1", {{{1}, 1}}},
        IPv6PrefixParseTestCase{"0:0:0:0:0:0:0:0/1", {{{}, 1}}},
        IPv6PrefixParseTestCase{"0:0:0:0::0:0:0/1", {{{}, 1}}},
        IPv6PrefixParseTestCase{"::0:0:0:0:0:0:0/1", {{{}, 1}}},
        IPv6PrefixParseTestCase{"0:0:0:0:0:0::0/1", {{{}, 1}}},
        IPv6PrefixParseTestCase{"0000:0000:0000:0000:0000:0000:0000:0000/1", {{{}, 1}}},
        IPv6PrefixParseTestCase{
            "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/1", {{{std::bitset<128>{}.set()}, 1}}},
        IPv6PrefixParseTestCase{"0:0:0:0:5:6:7::/1", {{{0x0005000600070000}, 1}}},
        IPv6PrefixParseTestCase{"::ffff:127.0.0.1/128", {{{0xffff7f000001}, 128}}},
        IPv6PrefixParseTestCase{"::ffff:100.100.100.100/64", {{{0xffff64646464}, 64}}},
        IPv6PrefixParseTestCase{"::ffff:255.255.255.255/32", {{{0xffffffffffff}, 32}}},
        IPv6PrefixParseTestCase{"::ffff:0.0.0.0/0", {{{0xffff00000000}, 0}}},
        IPv6PrefixParseTestCase{"::ffff:0.0.0.0%foo/0", std::nullopt},
        IPv6PrefixParseTestCase{"::1%% :x\x1F/0", std::nullopt},
        IPv6PrefixParseTestCase{"::/129", std::nullopt},
        IPv6PrefixParseTestCase{"::/-1", std::nullopt},
        IPv6PrefixParseTestCase{"", std::nullopt}),
    [](const testing::TestParamInfo<IPv6PrefixParseTestCase>& info) {
      std::string name = info.param.input;
      for (auto c : {':', '.', '/', '%', '\x1F', ' ', '-'}) {
        std::replace(name.begin(), name.end(), c, '_');
      }
      if (name.empty()) {
        return std::string{"Empty"};
      }
      return name;
    });

TEST(IPv6PrefixTest, Mask) {
  auto prefix = parseIPv6Prefix("1:2:3:4:5:6:7::/0");
  EXPECT_EQ(prefix.has_value(), true);
  EXPECT_EQ(prefix->prefixLength, 0);
  EXPECT_EQ(prefix->mask().none(), true);
  prefix = parseIPv6Prefix("1:2:3:4:5:6:7::/128");
  EXPECT_EQ(prefix.has_value(), true);
  EXPECT_EQ(prefix->prefixLength, 128);
  EXPECT_EQ(prefix->mask().all(), true);
}

} // namespace
} // namespace buf::validate::internal::lib
