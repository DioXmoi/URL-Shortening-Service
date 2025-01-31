#pragma once


#include <string>
#include <string_view>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <libpq-fe.h>


#include "IDatabase.h"


class PostgreSQL : public IDatabase {
public:

    PostgreSQL(
        std::string_view host = "127.0.0.1",
        std::string_view user = "postgres",
        std::string_view pass = "postgres",
        std::string_view dbName = "postgres", 
        int port = 5432, 
        int countConn = 1
    )
        : m_host{ host }
        , m_user{ user }
        , m_pass{ pass }
        , m_db_name{ dbName }
        , m_port{ std::to_string(port) }
        , m_pool{ countConn }
    {
        if (m_host.empty()) {
            throw std::invalid_argument("The value of the host field cannot be empty");
        }

        if (m_user.empty()) {
            throw std::invalid_argument("The value of the user field cannot be empty");
        }

        if (m_pass.empty()) {
            throw std::invalid_argument("The value of the password field cannot be empty");
        }

        if (m_db_name.empty()) {
            throw std::invalid_argument("The value of the dbname field cannot be empty");
        }

        if (port < 0 || port > 65535) {
            throw std::invalid_argument("The value of the port field must be in the range from 0 to 65535");
        }

        if (countConn < 1) {
            throw std::invalid_argument("The number of database connections cannot be less than one");
        }
    }

    void Connect() override {

        const char* keywords[] = { "host", "user", "password", "dbname", "port", nullptr };
        const char* values[] = { m_host.c_str(), m_user.c_str(), m_pass.c_str(), m_db_name.c_str(), m_port.c_str(), nullptr };

        m_pool.Connect(keywords, values);
    }

    void Disconnect() override {
        m_pool.Disconnect();
    }

    void Execute(std::string_view query, SqlParams params) override {

        auto lengths{ GetLengthsParams(params) };
        auto values{ GetValuesParams(params) };

        auto conn{ m_pool.Acquire() };

        std::unique_ptr<PGresult, decltype(&PQclear)> res_guard(
            PQexecParams(conn.get(), 
                         query.data(), 
                         params.size(), 
                         nullptr, 
                         values.data(), 
                         lengths.data(), 
                         nullptr, 
                         0), PQclear);

        if (PQresultStatus(res_guard.get()) != PGRES_COMMAND_OK) {
            m_pool.Release(std::move(conn));
            return;
        }

        m_pool.Release(std::move(conn));
    }

    std::vector<std::string> ExecuteQuery(std::string_view query, SqlParams params) override {

        auto lengths{ GetLengthsParams(params) };
        auto values{ GetValuesParams(params) };

        auto conn{ m_pool.Acquire() };
        std::unique_ptr<PGresult, decltype(&PQclear)> res_guard{
            PQexecParams(conn.get(),
                query.data(),
                params.size(),
                nullptr,
                values.data(),
                lengths.data(),
                nullptr,
                0), PQclear };

        if (PQresultStatus(res_guard.get()) != PGRES_TUPLES_OK) {
            m_pool.Release(std::move(conn));
            return { };
        }

        m_pool.Release(std::move(conn));

        std::vector<std::string> data{ };
        if (PQntuples(res_guard.get()) != 0) {
            int rows = PQntuples(res_guard.get());
            int cols = PQnfields(res_guard.get());
            data.reserve(rows * cols);
            for (int i{ 0 }; i < rows; ++i) {
                for (int j{ 0 }; j < cols; ++j) {
                    data.emplace_back(
                        PQgetvalue(res_guard.get(), i, j));
                }
            }
        }

        return data;
    }

    void BeginTransaction()override {

    }

    void CommitTransaction() override {

    }

    void RollbackTransaction() override {

    }

    ~PostgreSQL() = default;

private:

    static std::vector<int> GetLengthsParams(SqlParams params) {
        std::vector<int> lengths{ };
        for (const auto& param : params) {
            lengths.emplace_back(static_cast<int>(param.second.size()));
        }

        return lengths;
    }

    static std::vector<const char*> GetValuesParams(SqlParams params) {
        std::vector<const char*> values{ };
        for (const auto& param : params) {
            values.emplace_back(param.second.c_str());
        }

        return values;
    }

private:

    class ConnectionPool {
    public:

        ConnectionPool(int count) 
            : m_countConn{ count }
        {
        }

        void Connect(const char* keywords[], const char* values[]) {
            std::lock_guard<std::mutex> lock(m_mutex);

            m_connections.clear();
            for (int i = 0; i < m_countConn; ++i) {
                PGconn* conn{ PQconnectdbParams(keywords, values, 0) };
                if (PQstatus(conn) != CONNECTION_OK) {
                    std::string message = PQerrorMessage(conn);
                    std::cerr << "Connection failed: " << message << "\n";
                    PQfinish(conn);
                    return;
                }

                m_connections.emplace_back(conn, &PQfinish);
            }
        }

        void Disconnect() {
            for (auto& conn : m_connections) {
                if (conn.get() != nullptr) {
                    PQfinish(conn.get());
                }
            }

            m_connections.clear();
        }

        int Count() const { return m_countConn; }

        void SetCountConnections(int newCount) {
            if (newCount >= 1) {
                m_countConn = newCount;
            }
        }

        std::unique_ptr<PGconn, decltype(&PQfinish)> Acquire() {
            std::unique_lock<std::mutex> lock{ m_mutex };
            m_cond.wait(lock, [this]() { return !m_connections.empty(); });
            auto conn{ std::move(m_connections.back()) };
            m_connections.pop_back();

            return conn;
        }

        void Release(std::unique_ptr<PGconn, decltype(&PQfinish)> conn) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_connections.push_back(std::move(conn));
            m_cond.notify_one();
        }

    private:
        int m_countConn{ };
        std::vector<std::unique_ptr<PGconn, decltype(&PQfinish)>> m_connections;

        std::mutex m_mutex{ };
        std::condition_variable m_cond{ };
    };

private:

    const std::string m_host{ };
    const std::string m_user{ };
    const std::string m_pass{ };
    const std::string m_db_name{ };
    const std::string m_port{ };

    ConnectionPool m_pool{ 1 };
};