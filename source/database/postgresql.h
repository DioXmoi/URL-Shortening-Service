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

    }

    void Connect() override {
        const char* keywords[] = { "host", "user", "password", "dbname", "port", nullptr };
        const char* values[] = { m_config.GetHost().c_str(),
                                 m_config.GetUser().c_str(), 
                                 m_config.GetPassword().c_str(), 
                                 m_config.GetDatabaseName().c_str(), 
                                 m_config.GetPort().c_str(), 
                                 nullptr };

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

        ConnectionPool(int countConn)
            : m_countConn{ countConn }
        {
            if (countConn < 1) {
                throw std::invalid_argument("Number of connections must be >= 1.");
            }
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


    class ConnectionConfig {
    public:
        ConnectionConfig(std::string_view host,
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
                throw std::invalid_argument("Field host cannot be empty.");
            }

            if (m_user.empty()) {
                throw std::invalid_argument("Field user cannot be empty.");
            }

            if (m_pass.empty()) {
                throw std::invalid_argument("Field password cannot be empty.");
            }

            if (m_dbName.empty()) {
                throw std::invalid_argument("Field dbname cannot be empty.");
            }

            if (port < 0 || port > 65535) {
                throw std::invalid_argument("Field port must be a valid port number between 0 and 65535.");
            }
        }

        const std::string& GetHost() const { return m_host; }
        const std::string& GetUser() const { return m_user; }
        const std::string& GetPassword() const { return m_pass; }
        const std::string& GetDatabaseName() const { return m_dbName; }
        const std::string& GetPort() const { return m_port; }

    private:

        const std::string m_host;
        const std::string m_user;
        const std::string m_pass;
        const std::string m_dbName;
        const std::string m_port;
    };

private:

    ConnectionConfig m_config;
    ConnectionPool m_pool;
};