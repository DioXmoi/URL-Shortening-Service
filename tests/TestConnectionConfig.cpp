#include <gtest/gtest.h>

#include <postgresql.h>


TEST(TestConnectionConfig, ValidParams) {
    PostgreSQL::ConnectionConfig config("localhost", "user", "pass", "db", 5432);

    EXPECT_EQ(config.GetHost(), "localhost");
    EXPECT_EQ(config.GetUser(), "user");
    EXPECT_EQ(config.GetPassword(), "pass");
    EXPECT_EQ(config.GetDatabaseName(), "db");
    EXPECT_EQ(config.GetPort(), "5432"); // The port is converted to the string.
}

TEST(TestConnectionConfig, EmptyParams) {
    EXPECT_THROW({
        PostgreSQL::ConnectionConfig emptyHost("", "user", "pass", "db", 5432);
    }, PostgreSQL::ConnectionConfigError);

    EXPECT_THROW({
        PostgreSQL::ConnectionConfig emptyUser("localhost", "", "pass", "db", 5432);
        }, PostgreSQL::ConnectionConfigError);

    EXPECT_THROW({
        PostgreSQL::ConnectionConfig emptyPassword("localhost", "user", "", "db", 5432);
        }, PostgreSQL::ConnectionConfigError);

    EXPECT_THROW({
        PostgreSQL::ConnectionConfig emptyDatabaseName("localhost", "user", "pass", "", 5432);
        }, PostgreSQL::ConnectionConfigError);

    EXPECT_THROW({
        PostgreSQL::ConnectionConfig portLessThenZero("localhost", "user", "pass", "db", -1);
        }, PostgreSQL::ConnectionConfigError);

    EXPECT_THROW({
        constexpr int max_port{ 65535 };
        PostgreSQL::ConnectionConfig portGreaterThanMaxValid("localhost", "user", "pass", "db", max_port + 1);
        }, PostgreSQL::ConnectionConfigError);
}