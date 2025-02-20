#include <gmock/gmock.h> 
#include <gtest/gtest.h>
#include "postgresql.h"


class MockPGClient : public PostgreSQL::IPGClient {
public:
    MOCK_METHOD(PGconn*, PQconnectdbParams, (const char* const*, const char* const*, int), (override));
    MOCK_METHOD(PGresult*, PQexecParams, (PGconn*, const char*, int, const Oid*, const char* const*, const int*, const int*, int), (override));
    MOCK_METHOD(ConnStatusType, PQstatus, (const PGconn*), (override));
    MOCK_METHOD(char*, PQerrorMessage, (const PGconn*), (override));
    MOCK_METHOD(void, PQfinish, (PGconn*), (override));
    MOCK_METHOD(void, PQreset, (PGconn*), (override));
    MOCK_METHOD(ExecStatusType, PQresultStatus, (const PGresult*), (override));
    MOCK_METHOD(void, PQclear, (PGresult*), (override));
    MOCK_METHOD(int, PQntuples, (const PGresult*), (override));
    MOCK_METHOD(int, PQnfields, (const PGresult*), (override));
    MOCK_METHOD(char*, PQgetvalue, (const PGresult*, int, int), (override));
    MOCK_METHOD(int, PQgetisnull, (const PGresult*, int, int), (override));
    MOCK_METHOD(int, PQgetlength, (const PGresult*, int, int), (override));
};