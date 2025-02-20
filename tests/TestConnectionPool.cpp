#include <gtest/gtest.h>

#include <future>
#include <thread>
#include "postgresql.h"
#include "TestConfig.h"
#include "TestConnectionPool.h"


using::testing::_;
using ::testing::AtLeast;
using ::testing::Return;

static PostgreSQL::ConnectionConfig config{
		__HOST_DATABASE, 
		__USER_DATABASE, 
		__PASSWORD_DATABASE, 
		__NAME_DATABASE, 
		__PORT_DATABASE };

static PostgreSQL::ConnectionParams params{ config.GetConnectionStringParams() };


TEST(ConnectionPoolTest, CreatesPoolWithValidParams) {
	auto ptr{ std::make_shared<MockPGClient>() };
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(2));

	EXPECT_CALL(*ptr, PQstatus(_))
		.WillOnce(Return(CONNECTION_OK))
		.WillOnce(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQerrorMessage(_))
		.Times(AtLeast(0));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	EXPECT_NO_THROW({	
		auto pool = PostgreSQL::ConnectionPool(params, client, 2);

		EXPECT_EQ(pool.Count(), 2);
	});
}

TEST(ConnectionPoolTest, CreatesPoolWithInvalidParams) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(0));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	EXPECT_THROW({
		auto pool = PostgreSQL::ConnectionPool(params, client, 0);

	}, PostgreSQL::ConnectionPoolError);

	EXPECT_THROW({	
		auto pool = PostgreSQL::ConnectionPool(params, client, -1);

	}, PostgreSQL::ConnectionPoolError);
}

TEST(ConnectionPoolTest, ConnectCalledAgainMaintainsConnectionCount) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(4));

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(AtLeast(4))
		.WillRepeatedly(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQerrorMessage(_))
		.Times(AtLeast(0));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;

	// The Connect constructor is implicitly called
	auto pool = PostgreSQL::ConnectionPool(params, client, 2);
	EXPECT_EQ(pool.Count(), 2);

	pool.Connect(params);

	EXPECT_EQ(pool.Count(), 2);
}

TEST(ConnectionPoolTest, InvalidConnectionThrowsConnectError) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(1));

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(AtLeast(1))
		.WillRepeatedly(Return(CONNECTION_BAD));

	EXPECT_CALL(*ptr, PQerrorMessage(_))
		.Times(AtLeast(1));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	// The Connect constructor is implicitly called
	EXPECT_THROW({
		auto pool = PostgreSQL::ConnectionPool(params, client, 2);

		EXPECT_EQ(pool.Count(), 0);
	}, PostgreSQL::ConnectError);
}

TEST(ConnectionPoolTest, DisconnectClosesAllConnections) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(reinterpret_cast<PGconn*>(0x1))); // Dummy pointer;

	EXPECT_CALL(*ptr, PQstatus(_))
		.WillOnce(Return(CONNECTION_OK))
		.WillOnce(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(2));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	auto pool = PostgreSQL::ConnectionPool(params, client, 2);

	pool.Disconnect();

	EXPECT_EQ(pool.Count(), 0);
}

TEST(ConnectionPoolTest, ReconnectAfterDisconnect) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(4));

	EXPECT_CALL(*ptr, PQstatus(_))
		.WillRepeatedly(Return(CONNECTION_OK));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	PostgreSQL::ConnectionPool pool(params, client, 2);

	pool.Disconnect();
	EXPECT_EQ(pool.Count(), 0);

	pool.Connect(params);
	EXPECT_EQ(pool.Count(), 2);
}

TEST(ConnectionPoolTest, MaxConnectionsLimit) {
	auto ptr = std::make_shared<MockPGClient>();
	const int MAX_CONN = 100;
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(MAX_CONN))
		.WillRepeatedly(Return(reinterpret_cast<PGconn*>(0x1)));

	EXPECT_CALL(*ptr, PQstatus(_))
		.WillRepeatedly(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(MAX_CONN));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	PostgreSQL::ConnectionPool pool(params, client, MAX_CONN);
	EXPECT_EQ(pool.Count(), MAX_CONN);
}

TEST(ConnectionPoolTest, ConnectionLifetime) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(reinterpret_cast<PGconn*>(0x1)));

	EXPECT_CALL(*ptr, PQstatus(_))
		.WillRepeatedly(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(2))
		.WillRepeatedly(testing::Invoke([](PGconn* conn) {
				ASSERT_NE(conn, nullptr) << "Wrong! 'PQfinish' was not called.\n";
			}));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	{
		PostgreSQL::ConnectionPool pool(params, client, 2);
		EXPECT_EQ(pool.Count(), 2);
	}
}

TEST(ConnectionPoolTest, AcquireReturnsConnectionWhenAvailable) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.WillOnce(Return(reinterpret_cast<PGconn*>(0x1))); // Dummy pointer;

	EXPECT_CALL(*ptr, PQstatus(_))
		.WillOnce(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(1));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	auto pool = PostgreSQL::ConnectionPool(params, client, 1);

	auto conn = pool.Acquire();

	EXPECT_NE(conn.get(), nullptr);
}

TEST(ConnectionPoolTest, AcquireDecreasesCountWhenConnectionAcquired) {
	auto ptr = std::make_shared<MockPGClient>();

	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(2)
		.WillRepeatedly(Return(reinterpret_cast<PGconn*>(0x1))); // Dummy pointer;

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(2)
		.WillRepeatedly(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(2));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	auto pool = PostgreSQL::ConnectionPool(params, client, 2);

	auto conn1 = pool.Acquire();
	EXPECT_EQ(pool.Count(), 1);

	auto conn2 = pool.Acquire();
	EXPECT_EQ(pool.Count(), 0);
}

TEST(ConnectionPoolTest, AcquireWhenPoolEmptyThrowsConnectionPoolError) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(2)
		.WillRepeatedly(Return(reinterpret_cast<PGconn*>(0x1))); // Dummy pointer;

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(2)
		.WillRepeatedly(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(2));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	auto pool = PostgreSQL::ConnectionPool(params, client, 2);

	auto conn1 = pool.Acquire();
	auto conn2 = pool.Acquire();

	EXPECT_THROW({
		auto conn3 = pool.Acquire();
	}, PostgreSQL::ConnectionPoolError);
}

TEST(ConnectionPoolTest, ReleaseReturnsConnectionToPool) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.WillOnce(Return(reinterpret_cast<PGconn*>(0x1))); // Dummy pointer;

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(1));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	auto pool = PostgreSQL::ConnectionPool(params, client, 1);

	auto conn = pool.Acquire();
	EXPECT_EQ(pool.Count(), 0);

	EXPECT_NE(conn.get(), nullptr);

	pool.Release(std::move(conn));
	EXPECT_EQ(pool.Count(), 1);
}

TEST(ConnectionPoolTest, ReleaseRecoversInvalidConnection) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(1));

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(AtLeast(2))
		.WillOnce(Return(CONNECTION_OK))
		.WillOnce(Return(CONNECTION_BAD))
		.WillRepeatedly(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQreset(_))
		.Times(AtLeast(1));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	auto pool = PostgreSQL::ConnectionPool(params, client, 1);

	auto conn = pool.Acquire();
	EXPECT_EQ(pool.Count(), 0);
	pool.Release(std::move(conn));
	EXPECT_EQ(pool.Count(), 1);
}

TEST(ConnectionPoolTest, ReleaseResetFailsThrowsResetError) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(1));

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(AtLeast(1))
		.WillOnce(Return(CONNECTION_OK))
		.WillRepeatedly(Return(CONNECTION_BAD));

	EXPECT_CALL(*ptr, PQreset(_))
		.Times(AtLeast(1));

	EXPECT_CALL(*ptr, PQerrorMessage(_))
		.Times(AtLeast(1));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	auto pool = PostgreSQL::ConnectionPool(params, client, 1);

	auto conn = pool.Acquire();
	EXPECT_EQ(pool.Count(), 0);

	EXPECT_THROW({
		pool.Release(std::move(conn));
		
		EXPECT_EQ(pool.Count(), 0);
	}, PostgreSQL::ResetError);
}

TEST(ConnectionPoolTest, ConcurrentAcquireReleaseWorksCorrectly) {
	auto ptr = std::make_shared<MockPGClient>();
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(reinterpret_cast<PGconn*>(0x1))); // Dummy pointer;

	EXPECT_CALL(*ptr, PQstatus(_))
		.WillRepeatedly(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(2));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	auto pool = PostgreSQL::ConnectionPool(params, client, 2);

	EXPECT_EQ(pool.Count(), 2);

	std::promise<void> threadPromise;
	std::future<void> threadFuture{ threadPromise.get_future() };

	std::thread thread{ [&]() {
		try {
			auto conn1 = pool.Acquire();
			std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
			auto conn2 = pool.Acquire();
			std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
			pool.Release(std::move(conn1));
			pool.Release(std::move(conn2));
			std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
			conn1 = pool.Acquire();
			conn2 = pool.Acquire();
			pool.Release(std::move(conn1));
			pool.Release(std::move(conn2));

			threadPromise.set_value();
		}
		catch (const std::exception& e) {
			FAIL() << "Thread threw an exception: " << e.what();
			threadPromise.set_exception(std::current_exception());
		}
	}};

	try {
		auto conn = pool.Acquire();
		std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
		pool.Release(std::move(conn));
	}
	catch (const std::exception& e) {
		FAIL() << "Main thread threw an exception: " << e.what();
	}

	threadFuture.get();
	thread.join();

	EXPECT_EQ(pool.Count(), 2);
}

TEST(ConnectionPoolTest, HighLoad) {
	auto ptr = std::make_shared<MockPGClient>();
	const int THREADS = 50;
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(THREADS))
		.WillRepeatedly(Return(reinterpret_cast<PGconn*>(0x1)));

	EXPECT_CALL(*ptr, PQstatus(_))
		.WillRepeatedly(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(THREADS));

	std::shared_ptr<PostgreSQL::IPGClient> client = ptr;
	PostgreSQL::ConnectionPool pool(params, client, THREADS);

	std::vector<std::thread> threads;
	for (int i = 0; i < THREADS; ++i) {
		threads.emplace_back([&]() {
			auto conn = pool.Acquire();
			std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
			pool.Release(std::move(conn));
		});
	}

	for (auto& t : threads) {
		t.join();
	}

	EXPECT_EQ(pool.Count(), THREADS);
}