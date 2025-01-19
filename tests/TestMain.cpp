#include <gtest/gtest.h>

TEST(MyTestSuite, MyFirstTest) {
    ASSERT_EQ(2 + 2, 4);
}

TEST(MyTestSuite, MySecondTest) {
    ASSERT_NE(5, 7);
}