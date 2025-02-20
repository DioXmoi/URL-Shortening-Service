#pragma once


#include <functional>
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
    using PGresultPtr = std::unique_ptr<PGresult, std::function<void(PGresult* res)>>;
    using PGconnPtr = std::unique_ptr<PGconn, std::function<void(PGconn* conn)>>;
    using ConnectionParams = std::pair<std::vector<const char*>, std::vector<const char*>>;

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

        ConnectionParams GetConnectionStringParams();

    private:

        const std::string m_host;
        const std::string m_user;
        const std::string m_pass;
        const std::string m_dbName;
        const std::string m_port;
    };

    class IPGClient {
    public:

        virtual ~IPGClient() = default;
        // Connection
        virtual PGconn* PQconnectdbParams(
            const char* const* keywords,
            const char* const* values,
            int expand_dbname
        ) = 0;

        // Request Execution
        virtual PGresult* PQexecParams(
            PGconn* conn,
            const char* command,
            int nParams,
            const Oid* paramTypes,
            const char* const* paramValues,
            const int* paramLengths,
            const int* paramFormats,
            int resultFormat
        ) = 0;

        // Connection management
        virtual ConnStatusType PQstatus(const PGconn* conn) = 0;
        virtual char* PQerrorMessage(const PGconn* conn) = 0;
        virtual void PQfinish(PGconn* conn) = 0;
        virtual void PQreset(PGconn* conn) = 0;

        // Working with the result
        virtual ExecStatusType PQresultStatus(const PGresult* res) = 0;
        virtual void PQclear(PGresult* res) = 0;
        virtual int PQntuples(const PGresult* res) = 0;
        virtual int PQnfields(const PGresult* res) = 0;
        virtual char* PQgetvalue(const PGresult* res, int row, int col) = 0;
        virtual int PQgetisnull(const PGresult* res, int row, int col) = 0;
        virtual int PQgetlength(const PGresult* res, int row, int col) = 0;
    };

    class PGClient : public IPGClient {
    public:
        // Connection
        PGconn* PQconnectdbParams(
            const char* const* keywords,
            const char* const* values,
            int expand_dbname
        ) override;

        // Request Execution
        PGresult* PQexecParams(
            PGconn* conn,
            const char* command,
            int nParams,
            const Oid* paramTypes,
            const char* const* paramValues,
            const int* paramLengths,
            const int* paramFormats,
            int resultFormat
        ) override;

        // Connection management
        ConnStatusType PQstatus(const PGconn* conn) override;
        char* PQerrorMessage(const PGconn* conn) override;
        void PQfinish(PGconn* conn) override;
        void PQreset(PGconn* conn) override;

        // Working with the result
        ExecStatusType PQresultStatus(const PGresult* res) override;
        void PQclear(PGresult* res) override;
        int PQntuples(const PGresult* res) override;
        int PQnfields(const PGresult* res) override;
        char* PQgetvalue(const PGresult* res, int row, int col) override;
        int PQgetisnull(const PGresult* res, int row, int col) override;
        int PQgetlength(const PGresult* res, int row, int col) override;
    };

    class ConnectionPool {
    public:

        ConnectionPool(const ConnectionParams& params,
            std::shared_ptr<IPGClient>& client, int countConn = 1);

        void Connect(const ConnectionParams& params);

        void Disconnect();

        int Count() { 
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_connections.size(); 
        }

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

        std::shared_ptr<IPGClient> m_client;

        std::mutex m_mutex{ };
        std::condition_variable m_cond{ };
    };

    class Database : public IDatabase {
    public:
        Database(const ConnectionConfig& config,
            std::shared_ptr<IPGClient>&& client,
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
        std::shared_ptr<IPGClient> m_client;
        ConnectionPool m_pool;
    };
}