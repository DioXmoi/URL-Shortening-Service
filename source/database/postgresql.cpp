#include "postgresql.h"
#include <memory>

namespace PostgreSQL {

    ConnectionConfig::ConnectionConfig(std::string_view host,
        std::string_view user,
        std::string_view pass,
        std::string_view dbName,
        int port
    )
        : m_host{ host }
        , m_user{ user }
        , m_pass{ pass }
        , m_dbName{ dbName }
        , m_port{ std::to_string(port) }
    {
        if (m_host.empty()) {
            throw ConnectionConfigError("Field host cannot be empty.");
        }

        if (m_user.empty()) {
            throw ConnectionConfigError("Field user cannot be empty.");
        }

        if (m_pass.empty()) {
            throw ConnectionConfigError("Field password cannot be empty.");
        }

        if (m_dbName.empty()) {
            throw ConnectionConfigError("Field dbname cannot be empty.");
        }

        if (port < 0 || port > 65535) {
            throw ConnectionConfigError("Field port must be a valid port number between 0 and 65535.");
        }
    }

    ConnectionParams ConnectionConfig::GetConnectionStringParams() {
        std::vector<const char*> keywords{ "host", "user", "password", "dbname", "port", nullptr };
        std::vector<const char*> values{ m_host.c_str(), m_user.c_str(), m_pass.c_str(),
            m_dbName.c_str(), m_port.c_str(), nullptr };

        return std::make_pair(keywords, values);
    }

    Database::Database(
        const ConnectionConfig& config,
        std::shared_ptr<IPGClient>&& client,
        int countConn
    )
        : m_config{ config }
        , m_client{ client }
        , m_pool{ 
            m_config.GetConnectionStringParams(), 
            m_client,
            countConn
          }
    {
        Connect();
    }

    void Database::Connect() {
        auto params = m_config.GetConnectionStringParams();

        m_pool.Connect(params);
    }

    void Database::Disconnect() {
        m_pool.Disconnect();
    }

    std::vector<int>  Database::GetLengthsParams(SqlParams params) {
        std::vector<int> lengths{ };
        lengths.reserve(params.size());
        for (const auto& param : params) {
            if (param.second.empty()) {
                lengths.emplace_back(STR_NULL.size());
            }
            else {
                int len = static_cast<int>(param.second.size());
                lengths.emplace_back(len);
            }
        }

        return lengths;
    }

    std::vector<const char*>  Database::GetValuesParams(SqlParams params) {
        std::vector<const char*> values{ };
        values.reserve(params.size());
        for (const auto& param : params) {
            if (param.second.empty()) {
                values.emplace_back(STR_NULL.c_str());
            }
            else {
                const char* c_str = param.second.c_str();
                values.push_back(c_str);
            }
        }

        return values;
    }

    void Database::Execute(std::string_view query, SqlParams params) {
        auto lengths{ GetLengthsParams(params) };
        auto values{ GetValuesParams(params) };

        auto conn{ m_pool.Acquire() };

        PGresultPtr resGuard{
            m_client -> PQexecParams(conn.get(),
                query.data(),
                params.size(),
                nullptr,
                values.data(),
                lengths.data(),
                nullptr,
                0), [&](PGresult* res) -> void {
                    m_client -> PQclear(res);
                }};

        std::string msg_error{ m_client -> PQerrorMessage(conn.get()) };
        m_pool.Release(std::move(conn));
        if (m_client -> PQresultStatus(resGuard.get()) != PGRES_COMMAND_OK) {
            throw ExecuteError(msg_error);
        }
    }

    std::vector<std::string> Database::ReadPostgresResult(PGresultPtr resGuard) {
        std::vector<std::string> data{ };
        if (m_client -> PQntuples(resGuard.get()) != 0) {
            int rows = m_client -> PQntuples(resGuard.get());
            int cols = m_client -> PQnfields(resGuard.get());
            data.reserve(static_cast<std::size_t>(rows) * cols);
            for (int i{ 0 }; i < rows; ++i) {
                for (int j{ 0 }; j < cols; ++j) {
                    std::string str{ m_client -> PQgetvalue(resGuard.get(), i, j) };
                    if (str.empty()) {
                        str = STR_NULL;
                    }

                    data.emplace_back(std::move(str));
                }
            }
        }

        return data;
    }

    std::vector<std::string> Database::ExecuteQuery(std::string_view query, SqlParams params) {
        auto lengths{ GetLengthsParams(params) };
        auto values{ GetValuesParams(params) };

        auto conn{ m_pool.Acquire() };
        PGresultPtr resGuard{ m_client -> PQexecParams(conn.get(),
                query.data(),
                params.size(),
                nullptr,
                values.data(),
                lengths.data(),
                nullptr,
                0), [&](PGresult* res) { m_client -> PQclear(res); } };

        std::string msg_error{ m_client -> PQerrorMessage(conn.get()) };
        m_pool.Release(std::move(conn));
        if (m_client -> PQresultStatus(resGuard.get()) != PGRES_TUPLES_OK) {
            throw ExecuteError(msg_error);
        }

        return ReadPostgresResult(std::move(resGuard));
    }

    void Database::BeginTransaction() {
        //Not necessary yet
        throw std::runtime_error("Missing implementation");
    }

    void Database::CommitTransaction() {
        // Not necessary yet
        throw std::runtime_error("Missing implementation");
    }

    void Database::RollbackTransaction() {
        // Not necessary yet
        throw std::runtime_error("Missing implementation");
    }

    PGconn* PGClient::PQconnectdbParams(
        const char* const* keywords, 
        const char* const* values, 
        int expand_dbname
    ) {

        return ::PQconnectdbParams(keywords, values, 0);
    }

    PGresult* PGClient::PQexecParams(
        PGconn* conn, 
        const char* command, 
        int nParams, 
        const Oid* paramTypes, 
        const char* const* paramValues, 
        const int* paramLengths, 
        const int* paramFormats, 
        int resultFormat
    ) {
        return ::PQexecParams(conn, command, nParams, paramTypes, paramValues, 
            paramLengths, paramFormats, resultFormat);
    }

    ConnStatusType PGClient::PQstatus(const PGconn* conn) {
        return ::PQstatus(conn);
    }

    char* PGClient::PQerrorMessage(const PGconn* conn) {
        return ::PQerrorMessage(conn);
    }

    void PGClient::PQfinish(PGconn* conn) {
        if (conn != nullptr) {
            ::PQfinish(conn);
        }        

    }

    void PGClient::PQreset(PGconn* conn) {
        ::PQreset(conn);
    }

    ExecStatusType PGClient::PQresultStatus(const PGresult* res) {
        return ::PQresultStatus(res);
    }

    void PGClient::PQclear(PGresult* res) {
        ::PQclear(res);
    }

    int PGClient::PQntuples(const PGresult* res) {
        return ::PQntuples(res);
    }

    int PGClient::PQnfields(const PGresult* res) {
        return ::PQnfields(res);
    }

    char* PGClient::PQgetvalue(const PGresult* res, int row, int col) {
        return ::PQgetvalue(res, row, col);
    }

    int PGClient::PQgetisnull(const PGresult* res, int row, int col) {
        return ::PQgetisnull(res, row, col);
    }

    int PGClient::PQgetlength(const PGresult* res, int row, int col) {
        return ::PQgetlength(res, row, col);
    }

    ConnectionPool::ConnectionPool(const ConnectionParams& params, 
        std::shared_ptr<IPGClient>& client, int countConn
    )
        : m_client{ client }
        , m_countConn{ countConn }
    {
        if (countConn < 1) {
            throw ConnectionPoolError("Number of connections must be >= 1.");
        }

        Connect(params);
    }

    void ConnectionPool::Connect(const ConnectionParams& params) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto keywords{ params.first.data()};
        auto values{ params.second.data() };
        m_connections.clear();

        for (int i = 0; i < m_countConn; ++i) {
            PGconnPtr conn{ m_client -> PQconnectdbParams(keywords, values, 0), 
                [&](PGconn* conn) -> void {
                    m_client -> PQfinish(conn);
                }};

            if (m_client -> PQstatus(conn.get()) != CONNECTION_OK) {
                m_connections.clear();
                throw ConnectError(m_client -> PQerrorMessage(conn.get()));
            }

            m_connections.emplace_back(std::move(conn));
        }
    }

    void ConnectionPool::Disconnect() {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto& conn : m_connections) {
            if (conn.get() != nullptr) {
                m_client -> PQfinish(conn.get());
                conn.reset(nullptr);
            }
        }

        m_connections.clear();
    }

    PGconnPtr ConnectionPool::Acquire() {
        const std::chrono::milliseconds waitInterval{ 100 }; // ToDo: Make it part of the customized interface 
        const std::chrono::milliseconds maxWaitTime{ 500 };
        const auto startTime = std::chrono::high_resolution_clock::now();

        std::unique_lock<std::mutex> lock{ m_mutex };
        while (m_connections.empty()) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
            if (elapsed >= maxWaitTime) {
                throw ConnectionPoolError{ "Timeout: Could not acquire a database connection from the pool within the allowed time." };
            }

            m_cond.wait_for(lock, waitInterval);
        }

        PGconnPtr conn{ std::move(m_connections.back()) };
        m_connections.pop_back();
        return conn;
    }

    void ConnectionPool::Release(PGconnPtr conn) {
        if (m_client -> PQstatus(conn.get()) != CONNECTION_OK) {
            m_client -> PQreset(conn.get());
            if (m_client -> PQstatus(conn.get()) != CONNECTION_OK) {
                throw ResetError(m_client -> PQerrorMessage(conn.get()));
            }
        }

        Push(std::move(conn));

        m_cond.notify_one();
    }
}