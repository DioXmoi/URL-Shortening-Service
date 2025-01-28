#pragma once

#include <any>
#include <string_view>
#include <vector>

#include <boost/system/error_code.hpp>



class IDatabase {
public:

    using SqlParams = const std::vector<std::pair<std::string, std::string>>&;

    virtual ~IDatabase() = default;
    virtual void connect(boost::system::error_code& ec) = 0;
    virtual void disconnect(boost::system::error_code& ec) = 0;
    virtual void execute(std::string_view query, SqlParams params, boost::system::error_code& ec) = 0;

    virtual std::any executeQuery(std::string_view query, SqlParams params, boost::system::error_code& ec) = 0;
    template <typename Response, typename Func>
    Response query(std::string_view query, SqlParams params, Func converter, boost::system::error_code& ec) {
        return converter(std::any_cast<std::string>(executeQuery(query, params, ec)));
    }

    virtual void beginTransaction(boost::system::error_code& ec) = 0;
    virtual void commitTransaction(boost::system::error_code& ec) = 0;
    virtual void rollbackTransaction(boost::system::error_code& ec) = 0;
};