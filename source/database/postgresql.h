#pragma once


#include <stdexcept>
#include <libpq-fe.h>
#include "IDatabase.h"


class PostgreSQL : public IDatabase {
public:

    PostgreSQL(int countConn = 1)
        : m_countConn{ countConn }
    {
        if (m_countConn < 1) {
            throw std::invalid_argument("The number of database connections cannot be less than one");
        }
    }

    void connect(boost::system::error_code& ec) override {

    }

    void disconnect(boost::system::error_code& ec) override {

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
    int m_countConn{ };
};