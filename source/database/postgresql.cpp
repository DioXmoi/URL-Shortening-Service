#include "postgresql.h"
#include <memory>

namespace PostgreSQL {

    ConnectionConfig::ConnectionConfig(std::string_view host,
        std::string_view user,
        std::string_view pass,
        std::string_view dbName,
        int port)
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


    Database::Database(
        std::string_view host,
        std::string_view user,
        std::string_view pass,
        std::string_view dbName,
        int port,
        int countConn
    )
        : m_config{ host, user, pass, dbName, port }
        , m_pool{ countConn }
    {
        Connect();
    }


    void Database::Connect() {
        const char* keywords[] = { "host", "user", "password", "dbname", "port", nullptr };
        const char* values[] = { m_config.GetHost().c_str(),
            m_config.GetUser().c_str(),
            m_config.GetPassword().c_str(),
            m_config.GetDatabaseName().c_str(),
            m_config.GetPort().c_str(),
            nullptr };

        m_pool.Connect(keywords, values);
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

        PGresultPtr resGuard(
            PQexecParams(conn.get(),
                query.data(),
                params.size(),
                nullptr,
                values.data(),
                lengths.data(),
                nullptr,
                0), PQclear);


        std::string msg_error{ PQerrorMessage(conn.get()) };
        m_pool.Release(std::move(conn));
        if (PQresultStatus(resGuard.get()) != PGRES_COMMAND_OK) {
            throw ExecuteError(msg_error);
        }
    }


    std::vector<std::string> Database::ReadPostgresResult(PGresultPtr resGuard) {
        std::vector<std::string> data{ };
        if (PQntuples(resGuard.get()) != 0) {
            int rows = PQntuples(resGuard.get());
            int cols = PQnfields(resGuard.get());
            data.reserve(static_cast<std::size_t>(rows) * cols);
            for (int i{ 0 }; i < rows; ++i) {
                for (int j{ 0 }; j < cols; ++j) {
                    std::string str{ PQgetvalue(resGuard.get(), i, j) };
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
        PGresultPtr resGuard{ PQexecParams(conn.get(),
                query.data(),
                params.size(),
                nullptr,
                values.data(),
                lengths.data(),
                nullptr,
                0), &PQclear };

        std::string msg_error{ PQerrorMessage(conn.get()) };
        m_pool.Release(std::move(conn));
        if (PQresultStatus(resGuard.get()) != PGRES_TUPLES_OK) {
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


    ConnectionPool::ConnectionPool(int countConn)
        : m_countConn{ countConn }
    {
        if (countConn < 1) {
            throw ConnectionPoolError("Number of connections must be >= 1.");
        }
    }


    void ConnectionPool::Connect(const char* keywords[], const char* values[]) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_connections.clear();
        for (int i = 0; i < m_countConn; ++i) {
            PGconnPtr conn{ PQconnectdbParams(keywords, values, 0), &PQfinish };

            if (PQstatus(conn.get()) != CONNECTION_OK) {
                m_connections.clear();
                throw ConnectError(PQerrorMessage(conn.get()));
            }

            m_connections.emplace_back(std::move(conn));
        }
    }


    void ConnectionPool::Disconnect() {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto& conn : m_connections) {
            if (conn.get() != nullptr) {
                PQfinish(conn.get());
            }
        }

        m_connections.clear();
    }


    PGconnPtr ConnectionPool::Acquire() {
        std::unique_lock<std::mutex> lock{ m_mutex };
        while (m_connections.empty()) {
            m_cond.wait(lock);
        }

        PGconnPtr conn{ std::move(m_connections.back()) };
        m_connections.pop_back();
        return conn;
    }


    void ConnectionPool::Release(PGconnPtr conn) {
        if (PQstatus(conn.get()) != CONNECTION_OK) {
            PQreset(conn.get());
            if (PQstatus(conn.get()) != CONNECTION_OK) {
                throw ResetError(PQerrorMessage(conn.get()));
            }
        }

        Push(std::move(conn));

        m_cond.notify_one();
    }
}