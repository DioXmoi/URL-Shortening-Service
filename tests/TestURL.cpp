#include <string>


#include <gtest/gtest.h>

#include "TestURL.h"



TEST_F(UrlTest, Init) {
    std::string uri{ url -> GetUri() };

    EXPECT_STREQ(uri.c_str(), "UrlTest.text");

    std::string shortCode{ url -> GetShortCode() };
    EXPECT_STREQ(shortCode.c_str(), "test");

    Id id{ url -> GetId() };
    EXPECT_EQ(id, 0);

    int accessCount{ url -> GetAccessCount() };
    EXPECT_EQ(accessCount, 0);

    auto created_at{ url -> GetCreatedAt() };
    auto updated_at{ url -> GetUpdatedAt() };
    EXPECT_TRUE(created_at.time_since_epoch().count() > 0);
    EXPECT_TRUE(updated_at.time_since_epoch().count() > 0);
}

TEST_F(UrlTest, SetShortCodeUpdatesTime) {
    std::string code{ "code" };

    auto beforeUpdate{ url -> GetUpdatedAt() };

    url -> SetShortCode(code);

    std::string shortCode{ url -> GetShortCode() };

    EXPECT_STREQ(code.c_str(), shortCode.c_str());

    auto afterUpdate{ url -> GetUpdatedAt() };

    EXPECT_TRUE(afterUpdate > beforeUpdate);
}

TEST_F(UrlTest, SetShortCodeEmpty) {
    std::string code{ "" };

    ASSERT_THROW(url -> SetShortCode(code), std::invalid_argument);
}

TEST_F(UrlTest, GetAccessCount) {
    int accessCount{ url -> GetAccessCount() };
    EXPECT_EQ(accessCount, 0);

    url -> IncrementAccessCount();

    accessCount = url -> GetAccessCount();

    EXPECT_EQ(accessCount, 1);
}