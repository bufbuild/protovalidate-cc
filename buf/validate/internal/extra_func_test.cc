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

#include "buf/validate/internal/extra_func.h"

#include "gtest/gtest.h"

TEST(ExtraFuncTest, TestIpPrefix) {
  EXPECT_TRUE(buf::validate::internal::IsIpPrefix("1.2.3.0/24", false));
  EXPECT_TRUE(buf::validate::internal::IsIpPrefix("1.2.3.4/24", false));
  EXPECT_TRUE(buf::validate::internal::IsIpPrefix("1.2.3.0/24", true));
  EXPECT_FALSE(buf::validate::internal::IsIpPrefix("1.2.3.4/24", true));
  EXPECT_TRUE(
      buf::validate::internal::IsIpPrefix("fd7a:115c:a1e0:ab12:4843:cd96:626b:4000/118", false));
  EXPECT_TRUE(
      buf::validate::internal::IsIpPrefix("fd7a:115c:a1e0:ab12:4843:cd96:626b:430b/118", false));
  EXPECT_FALSE(
      buf::validate::internal::IsIpPrefix("fd7a:115c:a1e0:ab12:4843:cd96:626b:430b/118", true));
  EXPECT_FALSE(buf::validate::internal::IsIpPrefix("1.2.3.4", false));
  EXPECT_FALSE(
      buf::validate::internal::IsIpPrefix("fd7a:115c:a1e0:ab12:4843:cd96:626b:430b", false));
  EXPECT_TRUE(buf::validate::internal::IsIpv4Prefix("1.2.3.0/24", false));
  EXPECT_TRUE(buf::validate::internal::IsIpv4Prefix("1.2.3.4/24", false));
  EXPECT_TRUE(buf::validate::internal::IsIpv4Prefix("1.2.3.0/24", true));
  EXPECT_FALSE(buf::validate::internal::IsIpv4Prefix("1.2.3.4/24", true));
  EXPECT_FALSE(
      buf::validate::internal::IsIpv4Prefix("fd7a:115c:a1e0:ab12:4843:cd96:626b:4000/118", false));
  EXPECT_TRUE(
      buf::validate::internal::IsIpv6Prefix("fd7a:115c:a1e0:ab12:4843:cd96:626b:4000/118", false));
  EXPECT_TRUE(
      buf::validate::internal::IsIpv6Prefix("fd7a:115c:a1e0:ab12:4843:cd96:626b:430b/118", false));
  EXPECT_TRUE(
      buf::validate::internal::IsIpv6Prefix("fd7a:115c:a1e0:ab12:4843:cd96:626b:4000/118", true));
  EXPECT_FALSE(
      buf::validate::internal::IsIpv6Prefix("fd7a:115c:a1e0:ab12:4843:cd96:626b:430b/118", true));
  EXPECT_FALSE(buf::validate::internal::IsIpv6Prefix("1.2.3.0/24", false));
  // Additional Tests for IsIpv6Prefix()
  EXPECT_TRUE(buf::validate::internal::IsIpv6Prefix("2001:db8::/29", true));
  EXPECT_FALSE(buf::validate::internal::IsIpv6Prefix("2001:db9::/29", true));
  EXPECT_TRUE(buf::validate::internal::IsIpv6Prefix("2001:db8:ff00::/40", true));
  EXPECT_FALSE(buf::validate::internal::IsIpv6Prefix("2001:db8:ff80::/40", true));
  EXPECT_TRUE(buf::validate::internal::IsIpv6Prefix("2001:db8:ff00:ff00::/57", true));
  EXPECT_TRUE(buf::validate::internal::IsIpv6Prefix("2001:db8:ff00:ff80::/57", true));
  EXPECT_FALSE(buf::validate::internal::IsIpv6Prefix("2001:db8:ff00:ffc0::/57", true));
}
