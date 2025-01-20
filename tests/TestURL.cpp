#include <string>


#include <gtest/gtest.h>

#include "TestURL.h"



TEST_F(UrlTest, test_url_init) {
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


TEST_F(UrlTest, test_url_set_short_code_updates_time) {
    std::string code{ "code" };

    auto beforeUpdate{ url -> GetUpdatedAt() };

    url -> SetShortCode(code);

    std::string shortCode{ url -> GetShortCode() };

    EXPECT_STREQ(code.c_str(), shortCode.c_str());

    auto afterUpdate{ url -> GetUpdatedAt() };

    EXPECT_TRUE(afterUpdate > beforeUpdate);
}


TEST_F(UrlTest, test_url_set_short_code_empty) {
    std::string code{ "" };

    ASSERT_THROW(url -> SetShortCode(code), std::invalid_argument);
}


TEST_F(UrlTest, test_url_get_access_count) {
    int accessCount{ url -> GetAccessCount() };
    EXPECT_EQ(accessCount, 0);

    url -> IncrementAccessCount();

    accessCount = url -> GetAccessCount();

    EXPECT_EQ(accessCount, 1);
}