#include <gtest/gtest.h>

#include "postgresql.h"
#include "TestConfig.h"


TEST(TestConnectionPool, ConstructorValidParams) {
	PostgreSQL::ConnectionConfig config{
		__HOST_DATABASE, __USER_DATABASE, __PASSWORD_DATABASE, __NAME_DATABASE, __PORT_DATABASE };

	auto params = config.GetConnectionStringParams();

	std::shared_ptr<PostgreSQL::IPGClient> client = std::make_shared<PostgreSQL::PGClient>();
	EXPECT_NO_THROW({	
		auto pool = PostgreSQL::ConnectionPool(params, client, 2);

		EXPECT_EQ(pool.Count(), 2);
	});
}

TEST(TestConnectionPool, ConstructorInvalidParams) {
	PostgreSQL::ConnectionConfig config{
		__HOST_DATABASE, __USER_DATABASE, __PASSWORD_DATABASE, __NAME_DATABASE, __PORT_DATABASE };

	auto params = config.GetConnectionStringParams();

	std::shared_ptr<PostgreSQL::IPGClient> client = std::make_shared<PostgreSQL::PGClient>();
	EXPECT_THROW({
		auto pool = PostgreSQL::ConnectionPool(params, client, 0);

	}, PostgreSQL::ConnectionPoolError);

	EXPECT_THROW({	
		auto pool = PostgreSQL::ConnectionPool(params, client, -1);

	}, PostgreSQL::ConnectionPoolError);
}

TEST(TestConnectionPool, TestMethodConnect) {
	PostgreSQL::ConnectionConfig config{
		__HOST_DATABASE, __USER_DATABASE, __PASSWORD_DATABASE, __NAME_DATABASE, __PORT_DATABASE };

	auto params = config.GetConnectionStringParams();
	std::shared_ptr<PostgreSQL::IPGClient> client = std::make_shared<PostgreSQL::PGClient>();

	auto pool = PostgreSQL::ConnectionPool(params, client, 2);

	pool.Connect(params);

	EXPECT_EQ(pool.Count(), 2);
}