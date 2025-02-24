#include <gtest/gtest.h>
#include <memory>
#include "TestConfig.h"
#include "MockPGClient.h"


using ::testing::_;
using ::testing::AtLeast;
using ::testing::Return;

using namespace PostgreSQL;

static ConnectionConfig config{
		__HOST_DATABASE,
		__USER_DATABASE,
		__PASSWORD_DATABASE,
		__NAME_DATABASE,
		__PORT_DATABASE };


void SetupPostgresTestEnvironment(MockPGClient* ptr, ExecStatusType status);

TEST(PostgresDatabaseTest, ExecuteSuccess) {
	auto ptr = std::make_shared<MockPGClient>();

	SetupPostgresTestEnvironment(ptr.get(), PGRES_COMMAND_OK);

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(1));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };

	EXPECT_NO_THROW({
		database.Execute("UPDATE tests SET count = count + 1;", { });
	});
}

TEST(PostgresDatabaseTest, ExecuteWithNullParam) {
	auto ptr = std::make_shared<MockPGClient>();
	PGconn* dummyConn = reinterpret_cast<PGconn*>(0x1);
	PGresult* dummyResult = reinterpret_cast<PGresult*>(0x1);
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(1))
		.WillOnce(Return(dummyConn));

	const char* expectedParams[]  = { "NULL" };
	const int   expectedNumParams = 1;
	EXPECT_CALL(*ptr, PQexecParams(_, _, 1, _, ConstCharPtrArrayEq(expectedParams, expectedNumParams), _, _, _))
		.Times(AtLeast(1))
		.WillOnce(Return(dummyResult));

	std::string msg_error{ };
	EXPECT_CALL(*ptr, PQerrorMessage(_))
		.Times(AtLeast(1))
		.WillOnce(Return(msg_error.data()));

	EXPECT_CALL(*ptr, PQresultStatus(_))
		.WillOnce(Return(PGRES_COMMAND_OK));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(1));

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(1));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };
	IDatabase::SqlParams params{ std::make_pair(std::string{ "$1" }, std::string{ "" }) };


	EXPECT_NO_THROW({
		database.Execute("INSERT INTO table VALUES ($1)", params);
	});
}

TEST(PostgresDatabaseTest, SyntaxErrorQueryThrowsExecuteError) {
	auto ptr = std::make_shared<MockPGClient>();

	PGconn* dummyConn = reinterpret_cast<PGconn*>(0x1);
	PGresult* dummyResult = reinterpret_cast<PGresult*>(0x1);
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(1))
		.WillOnce(Return(dummyConn));

	EXPECT_CALL(*ptr, PQexecParams(_, _, _, _, _, _, _, _))
		.Times(AtLeast(1))
		.WillOnce(Return(dummyResult));

	std::string msg_error{ };
	EXPECT_CALL(*ptr, PQerrorMessage(_))
		.Times(AtLeast(1))
		.WillOnce(Return(msg_error.data()));

	EXPECT_CALL(*ptr, PQresultStatus(_))
		.WillOnce(Return(PGRES_FATAL_ERROR));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(1));

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(AtLeast(3))
		.WillOnce(Return(CONNECTION_OK))
		.WillOnce(Return(CONNECTION_BAD))
		.WillOnce(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQreset(_))
		.Times(AtLeast(1));

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(1));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };

	EXPECT_THROW({
		database.Execute("No, I'm not a syntax error...;", { });
	}, ExecuteError);
}

TEST(PostgresDatabaseTest, DataTypeErrorQueryThrowsExecuteError) {
	auto ptr = std::make_shared<MockPGClient>();

	PGconn* dummyConn = reinterpret_cast<PGconn*>(0x1);
	PGresult* dummyResult = reinterpret_cast<PGresult*>(0x1);
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(1))
		.WillOnce(Return(dummyConn));

	EXPECT_CALL(*ptr, PQexecParams(_, _, _, _, _, _, _, _))
		.Times(AtLeast(1))
		.WillOnce(Return(dummyResult));

	std::string msg_error{ };
	EXPECT_CALL(*ptr, PQerrorMessage(_))
		.Times(AtLeast(1))
		.WillOnce(Return(msg_error.data()));

	EXPECT_CALL(*ptr, PQresultStatus(_))
		.WillOnce(Return(PGRES_FATAL_ERROR));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(1));

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(AtLeast(3))
		.WillOnce(Return(CONNECTION_OK))
		.WillOnce(Return(CONNECTION_BAD))
		.WillOnce(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQreset(_))
		.Times(AtLeast(1));

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(1));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };

	auto params = std::vector<std::pair<std::string, std::string>>{ 
		std::make_pair(
			std::string{ "$1" }, 
			std::string{ "It should be an INT, not a string." }
		)};
	EXPECT_THROW({
		database.Execute("UPDATE tests SET count = $1;", params);
		}, ExecuteError);
}

TEST(PostgresDatabaseTest, ExecuteQuerySuccess) {
	auto ptr = std::make_shared<MockPGClient>();
		
	SetupPostgresTestEnvironment(ptr.get(), PGRES_TUPLES_OK);

	EXPECT_CALL(*ptr, PQntuples(_))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(1));

	EXPECT_CALL(*ptr, PQnfields(_))
		.Times(AtLeast(1))
		.WillRepeatedly(Return(3));

	std::vector<std::string> values{ std::string{ "First" },  std::string{ "Second" },  std::string{ "Third" }, };
	EXPECT_CALL(*ptr, PQgetvalue(_, _, _))
		.Times(AtLeast(3))
		.WillOnce(Return(values[0].data()))
		.WillOnce(Return(values[1].data()))
		.WillOnce(Return(values[2].data()));

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(1));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };

	EXPECT_NO_THROW({
		auto res = database.ExecuteQuery("SELECT * FROM tests;", { });

		EXPECT_EQ(values, res);
	});
}

TEST(PostgresDatabaseTest, SyntaxErrorExecuteQueryThrowsExecuteError) {
	auto ptr = std::make_shared<MockPGClient>();

	SetupPostgresTestEnvironment(ptr.get(), PGRES_FATAL_ERROR);

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(1));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };
	EXPECT_THROW({
		auto res = database.ExecuteQuery("GET * FROM tests WHAT count ___SUM(BAD_TEST);", { });
	}, ExecuteError);
}

TEST(PostgresDatabaseTest, ReadPostgresResultEmptyResult) {
	auto ptr = std::make_shared<MockPGClient>();

	SetupPostgresTestEnvironment(ptr.get(), PGRES_TUPLES_OK);

	EXPECT_CALL(*ptr, PQntuples(_))
		.Times(AtLeast(1))
		.WillOnce(Return(0));

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(1));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };

	auto res = database.ExecuteQuery("SELECT * FROM tests WHERE id = 123456789;", { });

	EXPECT_TRUE(res.empty());
}

TEST(PostgresDatabaseTest, ReadPostgresResultWithNull) {
	auto ptr = std::make_shared<MockPGClient>();

	SetupPostgresTestEnvironment(ptr.get(), PGRES_TUPLES_OK);

	EXPECT_CALL(*ptr, PQntuples(_))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(1));

	EXPECT_CALL(*ptr, PQnfields(_))
		.Times(AtLeast(1))
		.WillRepeatedly(Return(3));

	std::vector<std::string> values{ std::string{ "First" },  std::string{ "NOTNULL" },  std::string{ "" }, };
	// PQgetvalue function returns an empty string if the field value is NULL.
	EXPECT_CALL(*ptr, PQgetvalue(_, _, _))
		.Times(AtLeast(3))
		.WillOnce(Return(values[0].data()))
		.WillOnce(Return(values[1].data()))
		.WillOnce(Return(values[2].data())); 

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(1));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };

	auto res = database.ExecuteQuery("SELECT * FROM tests;", { });

	EXPECT_EQ(std::string{ "NULL" }, res.back());
}

TEST(PostgresDatabaseTest, ConnectionReuse) {
	auto ptr = std::make_shared<MockPGClient>();

	PGconn* dummyConn = reinterpret_cast<PGconn*>(0x1);
	PGresult* dummyResult = reinterpret_cast<PGresult*>(0x1);
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(1))
		.WillOnce(Return(dummyConn));

	EXPECT_CALL(*ptr, PQexecParams(_, _, _, _, _, _, _, _))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(dummyResult));

	std::string msg_error{ };
	EXPECT_CALL(*ptr, PQerrorMessage(_))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(msg_error.data()));

	EXPECT_CALL(*ptr, PQresultStatus(_))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(PGRES_COMMAND_OK));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(1));

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(AtLeast(3))
		.WillRepeatedly(Return(CONNECTION_OK));

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(2));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };

	database.Execute("QUERY1", { });
	database.Execute("QUERY2", { });
}

TEST(PostgresDatabaseTest, ExecuteQueryZeroColumns) {
	auto ptr = std::make_shared<MockPGClient>();

	SetupPostgresTestEnvironment(ptr.get(), PGRES_TUPLES_OK);

	EXPECT_CALL(*ptr, PQntuples(_))
		.Times(AtLeast(1))
		.WillRepeatedly(Return(0));

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(1));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };

	auto res = database.ExecuteQuery("SELECT * FROM tests;", { });

	EXPECT_TRUE(res.empty());
}

TEST(PostgresDatabaseTest, ExecuteQueryOneColumns) {
	auto ptr = std::make_shared<MockPGClient>();

	SetupPostgresTestEnvironment(ptr.get(), PGRES_TUPLES_OK);

	EXPECT_CALL(*ptr, PQntuples(_))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(1));

	EXPECT_CALL(*ptr, PQnfields(_))
		.Times(AtLeast(1))
		.WillRepeatedly(Return(3));

	std::vector<std::string> expected{ std::string{ "one" },  std::string{ "two" },  std::string{ "I`m ready!" } };
	EXPECT_CALL(*ptr, PQgetvalue(_, _, _))
		.Times(AtLeast(3))
		.WillOnce(Return(expected[0].data()))
		.WillOnce(Return(expected[1].data()))
		.WillOnce(Return(expected[2].data()));

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(1));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };

	auto res = database.ExecuteQuery("SELECT * FROM tests;", { });

	EXPECT_EQ(expected, res);
}

TEST(PostgresDatabaseTest, ExecuteQueryMultipleColumns) {
	auto ptr = std::make_shared<MockPGClient>();

	SetupPostgresTestEnvironment(ptr.get(), PGRES_TUPLES_OK);

	int colums = 5;
	EXPECT_CALL(*ptr, PQntuples(_))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(colums));

	int rows = 4;
	EXPECT_CALL(*ptr, PQnfields(_))
		.Times(AtLeast(1))
		.WillOnce(Return(rows));

	std::vector<std::string> expected{ std::string{ "Super Test" } };
	EXPECT_CALL(*ptr, PQgetvalue(_, _, _))
		.WillRepeatedly(Return(expected.front().data()));

	EXPECT_CALL(*ptr, PQclear(_))
		.Times(AtLeast(1));

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };

	auto res = database.ExecuteQuery("SELECT * FROM tests;", { });

	EXPECT_EQ(res.size(), colums * rows);
}

TEST(PostgresDatabaseTest, TransactionsNotImplemented) {
	auto ptr = std::make_shared<PGClient>();

	std::shared_ptr<IPGClient> client = ptr;
	auto database{ Database{ config, client } };

	EXPECT_THROW(database.BeginTransaction(), std::runtime_error);
	EXPECT_THROW(database.CommitTransaction(), std::runtime_error);
	EXPECT_THROW(database.RollbackTransaction(), std::runtime_error);
}


void SetupPostgresTestEnvironment(MockPGClient* ptr, ExecStatusType status) {
	PGconn* dummyConn = reinterpret_cast<PGconn*>(0x1);
	PGresult* dummyResult = reinterpret_cast<PGresult*>(0x1);
	EXPECT_CALL(*ptr, PQconnectdbParams(_, _, 0))
		.Times(AtLeast(1))
		.WillOnce(Return(dummyConn));

	EXPECT_CALL(*ptr, PQexecParams(_, _, _, _, _, _, _, _))
		.Times(AtLeast(1))
		.WillOnce(Return(dummyResult));

	std::string msg_error{ };
	EXPECT_CALL(*ptr, PQerrorMessage(_))
		.Times(AtLeast(1))
		.WillOnce(Return(msg_error.data()));

	EXPECT_CALL(*ptr, PQresultStatus(_))
		.WillOnce(Return(status));

	EXPECT_CALL(*ptr, PQfinish(_))
		.Times(AtLeast(1));

	EXPECT_CALL(*ptr, PQstatus(_))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(CONNECTION_OK));
}