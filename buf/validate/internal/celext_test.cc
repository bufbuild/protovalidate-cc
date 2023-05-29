#include "buf/validate/internal/celext.h"

#include "gtest/gtest.h"

namespace buf::validate::internal {
    namespace {

        class IsHostnameTest : public ::testing::Test {
        protected:
            CelExt fmt_;
        };

        TEST_F(IsHostnameTest, Numbers) {
            EXPECT_EQ(fmt_.isHostname("example.com"), true);
            EXPECT_EQ(fmt_.isHostname(".example.com"), false);
            EXPECT_EQ(fmt_.isHostname("EXAMPLE.COM"), true);
            EXPECT_EQ(
                    fmt_.isHostname("a-very-very-very-very-very-very-very-very-very-very-very-very-long-isHostname.com"), false);
            EXPECT_EQ(fmt_.isHostname("a-reasonably-long-isHostname-with-incorrect!ch@rs.com"), false);
        }

        class IsEmailTest : public ::testing::Test {
        protected:
            CelExt fmt_;
        };

        TEST_F(IsEmailTest, Numbers) {
            EXPECT_EQ(fmt_.isEmail("test@example.com"), true);
            EXPECT_EQ(fmt_.isEmail(".example.com"), false);
            EXPECT_EQ(fmt_.isEmail("@EXAMPLE.COM"), false);
        }

        class ValidateIPTest : public ::testing::Test {
        protected:
            CelExt fmt_;
        };

        TEST_F(ValidateIPTest, Numbers) {
            EXPECT_EQ(fmt_.validateIP("192.0.2.1", 4), true);
            EXPECT_EQ(fmt_.validateIP("192.0.2.1", 6), false);
            EXPECT_EQ(fmt_.validateIP("255.255.255.256", 4), false);
            EXPECT_EQ(fmt_.validateIP("2001:db8:3333:4444:5555:6666:7777:8888", 6), true);
            EXPECT_EQ(fmt_.validateIP("2001:db8:3333:4444:5555:6666:7777:8888", 4), false);
            EXPECT_EQ(fmt_.validateIP("2001:db8:a0b:12f0::::0:1", 6), false);
        }

    } // namespace
} // namespace buf::validate::internal