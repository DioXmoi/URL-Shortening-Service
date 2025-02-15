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
#include "postgresqlError.h"


namespace PostgreSQL {
    using PGresultPtr = std::unique_ptr<PGresult, decltype(&PQclear)>;
    using PGconnPtr = std::unique_ptr<PGconn, decltype(&PQfinish)>;


    class ConnectionConfig {
    public:
        ConnectionConfig(std::string_view host,
            std::string_view user,
            std::string_view pass,
            std::string_view dbName,
            int port);

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

    class ConnectionPool {
    public:

        ConnectionPool(int countConn);

        void Connect(const char* keywords[], const char* values[]);

        void Disconnect();

        int Count() const { return m_connections.size(); }

        PGconnPtr Acquire();

        void Release(PGconnPtr conn);

    private:

        void Push(PGconnPtr conn) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_connections.emplace_back(std::move(conn));
        }

    private:
        int m_countConn{ };
        std::vector<PGconnPtr> m_connections;

        std::mutex m_mutex{ };
        std::condition_variable m_cond{ };
    };

    class Database : public IDatabase {
    public:

        Database(std::string_view host,
            std::string_view user,
            std::string_view pass,
            std::string_view dbName,
            int port = 5432,
            int countConn = 1);

        void Connect() override;

        void Disconnect() override;

        void Execute(std::string_view query, SqlParams params) override;

        std::vector<std::string> ExecuteQuery(std::string_view query, SqlParams params) override;

        void BeginTransaction() override;

        void CommitTransaction() override;

        void RollbackTransaction() override;

        ~Database() = default;

    private:

        const std::string STR_NULL{ "NULL" };
        std::vector<int> GetLengthsParams(SqlParams params);
        std::vector<const char*> GetValuesParams(SqlParams params);

        std::vector<std::string> ReadPostgresResult(PGresultPtr resGuard);

    private:

        ConnectionConfig m_config;
        ConnectionPool m_pool;
    };
}