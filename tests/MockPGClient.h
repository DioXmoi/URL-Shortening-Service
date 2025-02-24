#include <gmock/gmock.h> 
#include <gtest/gtest.h>
#include "postgresql.h"


using ::testing::Matcher;
using ::testing::Args;
using ::testing::Eq;
using ::testing::StrEq;
using ::testing::ElementsAreArray;


class MockPGClient : public PostgreSQL::IPGClient {
public:
    MOCK_METHOD(PGconn*,        PQconnectdbParams,   (const char* const*, const char* const*, int), (override));
    MOCK_METHOD(PGresult*,      PQexecParams,        (PGconn*, const char*, int, const Oid*, const char* const*, const int*, const int*, int), (override));
    MOCK_METHOD(ConnStatusType, PQstatus,            (const PGconn*),   (override));
    MOCK_METHOD(char*,          PQerrorMessage,      (const PGconn*),   (override));
    MOCK_METHOD(void,           PQfinish,            (PGconn*),         (override));
    MOCK_METHOD(void,           PQreset,             (PGconn*),         (override));
    MOCK_METHOD(ExecStatusType, PQresultStatus,      (const PGresult*), (override));
    MOCK_METHOD(void,           PQclear,             (PGresult*),       (override));
    MOCK_METHOD(int,            PQntuples,           (const PGresult*), (override));
    MOCK_METHOD(int,            PQnfields,           (const PGresult*), (override));
    MOCK_METHOD(char*,          PQgetvalue,          (const PGresult*, int, int), (override));
    MOCK_METHOD(int,            PQgetisnull,         (const PGresult*, int, int), (override));
    MOCK_METHOD(int,            PQgetlength,         (const PGresult*, int, int), (override));

    ~MockPGClient() = default;
};

inline std::vector<std::string> ConvertConstCharPtrArrayToVector(const char* const* arr, int size) {
    std::vector<std::string> vec;
    if (arr) {
        for (int i = 0; i < size; ++i) {
            vec.push_back(arr[i]);
        }
    }
    return vec;
}

MATCHER_P2(ConstCharPtrArrayEq, expectedArr, expectedSize,
    "Checks if a const char* const* array matches the expected array") {
    if (arg == nullptr && expectedArr == nullptr) { return true; } // Both are null
    if (arg == nullptr || expectedArr == nullptr) { return false; } // One is null, the other isn't

    std::vector<std::string> actualVec = ConvertConstCharPtrArrayToVector(arg, expectedSize);
    std::vector<std::string> expectedVec = ConvertConstCharPtrArrayToVector(expectedArr, expectedSize);

    return ::testing::ExplainMatchResult(::testing::ElementsAreArray(expectedVec), actualVec, result_listener);
}