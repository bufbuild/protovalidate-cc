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

    } // namespace
} // namespace buf::validate::internal