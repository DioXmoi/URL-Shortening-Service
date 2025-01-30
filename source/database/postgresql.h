#pragma once


#include <string>
#include <string_view>
#include <stdexcept>
#include <iostream>
#include <vector>

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
        , m_countConn{ countConn }
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

        if (m_countConn < 1) {
            throw std::invalid_argument("The number of database connections cannot be less than one");
        }
    }

    void connect(boost::system::error_code& ec) override {
        ec = boost::system::error_code{ };

        const char* keywords[] = { "host", "user", "password", "dbname", "port", nullptr };
        const char* values[] = { m_host.c_str(), m_user.c_str(), m_pass.c_str(), m_db_name.c_str(), m_port.c_str(), nullptr };

        for (int i = m_connections.size(); i < m_countConn; ++i) {
            PGconn* conn{ PQconnectdbParams(keywords, values, 0) };
            if (PQstatus(conn) != CONNECTION_OK) {
                ec = boost::system::error_code(PQstatus(conn), boost::system::system_category());
                std::cerr << "Connection failed: " << PQerrorMessage(conn) << "\n";
                PQfinish(conn);
                return;
            }

            m_connections.emplace_back(conn, &PQfinish);
        }
    }

    void disconnect(boost::system::error_code& ec) override {
        ec = boost::system::error_code{ };
        for (auto& conn : m_connections) {
            if (conn.get() != nullptr) {
                PQfinish(conn.get());
            }
        }

        m_connections.clear();
    }

    void execute(std::string_view query, SqlParams params, boost::system::error_code& ec) override {

    }

    std::any executeQuery(std::string_view query, SqlParams params, boost::system::error_code& ec) override {
        return { };
    }

    void beginTransaction(boost::system::error_code& ec)override {

    }

    void commitTransaction(boost::system::error_code& ec) override {

    }

    void rollbackTransaction(boost::system::error_code& ec) override {

    }


private:

    const std::string m_host{ };
    const std::string m_user{ };
    const std::string m_pass{ };
    const std::string m_db_name{ };
    const std::string m_port{ };
    int m_countConn{ };

    std::vector<std::unique_ptr<PGconn, decltype(&PQfinish)>> m_connections;
};