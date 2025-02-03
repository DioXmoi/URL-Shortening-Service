#include "postgresql.h"


PostgreSQL::PostgreSQL(
    std::string_view host,
    std::string_view user,
    std::string_view pass,
    std::string_view dbName,
    int port = 5432,
    int countConn = 1
)
    : m_config{ host, user, pass, dbName, port }
    , m_pool{ countConn }
{
    Connect();
}


void PostgreSQL::Connect() {
    const char* keywords[] = { "host", "user", "password", "dbname", "port", nullptr };
    const char* values[] = { m_config.GetHost().c_str(), 
        m_config.GetUser().c_str(), 
        m_config.GetPassword().c_str(),
        m_config.GetDatabaseName().c_str(),
        m_config.GetPort().c_str(),
        nullptr };

    m_pool.Connect(keywords, values);
}


void PostgreSQL::Disconnect() {
    m_pool.Disconnect();
}


std::vector<int>  PostgreSQL::GetLengthsParams(SqlParams params) {
    std::vector<int> lengths{ };
    lengths.reserve(params.size());
    for (const auto& param : params) {
        if (param.second.empty()) {
            lengths.emplace_back(STR_NULL.size());
        }
        else {
            lengths.emplace_back(static_cast<int>(param.second.size()));
        }
    }

    return lengths;
}


std::vector<const char*>  PostgreSQL::GetValuesParams(SqlParams params) {
    std::vector<const char*> values{ };
    values.reserve(params.size());
    for (const auto& param : params) {
        if (param.second.empty()) {
            values.emplace_back(STR_NULL);
        }
        else {
            values.push_back(param.second.c_str());
        }
    }

    return values;
}


void PostgreSQL::Execute(std::string_view query, SqlParams params) {
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
        throw PostgreSQLError::ExecuteError(msg_error);
    }
}


std::vector<std::string> PostgreSQL::ReadPostgresResult(PGresultPtr resGuard) {
    std::vector<std::string> data{ };
    if (PQntuples(resGuard.get()) != 0) {
        int rows = PQntuples(resGuard.get());
        int cols = PQnfields(resGuard.get());
        data.reserve(rows * cols);
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


std::vector<std::string> PostgreSQL::ExecuteQuery(std::string_view query, SqlParams params) {
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
            0), PQclear };


    std::string msg_error{ PQerrorMessage(conn.get()) };
    m_pool.Release(std::move(conn)); 
    if (PQresultStatus(resGuard.get()) != PGRES_TUPLES_OK) {
        throw PostgreSQLError::ExecuteError(msg_error);
    }

    return ReadPostgresResult(std::move(resGuard));
}


void PostgreSQL::BeginTransaction() {
    //Not necessary yet
    throw std::runtime_error("Missing implementation");
}


void PostgreSQL::CommitTransaction() {
    // Not necessary yet
    throw std::runtime_error("Missing implementation");
}


void PostgreSQL::RollbackTransaction() {
    // Not necessary yet
    throw std::runtime_error("Missing implementation");
}


PostgreSQL::ConnectionPool::ConnectionPool(int countConn)
    : m_countConn{ countConn }
{
    if (countConn < 1) {
        throw PostgreSQLError::ConnectionPoolError("Number of connections must be >= 1.");
    }
}


void PostgreSQL::ConnectionPool::Connect(const char* keywords[], const char* values[]) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_connections.clear();
    for (int i = 0; i < m_countConn; ++i) {
        PGconnPtr conn{ PQconnectdbParams(keywords, values, 0), 
            &PQfinish };

        if (PQstatus(conn.get()) != CONNECTION_OK) {
            m_connections.clear();
            throw PostgreSQLError::ConnectError(PQerrorMessage(conn.get()));
        }

        m_connections.emplace_back(std::move(conn));
    }
}


void PostgreSQL::ConnectionPool::Disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& conn : m_connections) {
        if (conn.get() != nullptr) {
            PQfinish(conn.get());
        }
    }

    m_connections.clear();
}


PostgreSQL::PGconnPtr PostgreSQL::ConnectionPool::Acquire() {
    std::unique_lock<std::mutex> lock{ m_mutex };
    while (m_connections.empty()) {
        m_cond.wait(lock);
    }

    auto conn{ std::move(m_connections.back()) };
    m_connections.pop_back();

    return conn;
}


void PostgreSQL::ConnectionPool::Release(PGconnPtr conn) {
    if (PQstatus(conn.get()) != CONNECTION_OK) {
        PQreset(conn.get());
        if (PQstatus(conn.get()) != CONNECTION_OK) {
            throw PostgreSQLError::ResetError(PQerrorMessage(conn.get()));
        }
    }

    Push(std::move(conn));

    m_cond.notify_one();
}


PostgreSQL::ConnectionConfig::ConnectionConfig(std::string_view host,
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
        throw PostgreSQLError::ConnectionConfigError("Field host cannot be empty.");
    }

    if (m_user.empty()) {
        throw PostgreSQLError::ConnectionConfigError("Field user cannot be empty.");
    }

    if (m_pass.empty()) {
        throw PostgreSQLError::ConnectionConfigError("Field password cannot be empty.");
    }

    if (m_dbName.empty()) {
        throw PostgreSQLError::ConnectionConfigError("Field dbname cannot be empty.");
    }

    if (port < 0 || port > 65535) {
        throw PostgreSQLError::ConnectionConfigError("Field port must be a valid port number between 0 and 65535.");
    }
}