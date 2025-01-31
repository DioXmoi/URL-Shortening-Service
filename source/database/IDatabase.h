#pragma once

#include <string>
#include <string_view>
#include <vector>



class IDatabase {
public:

    using SqlParams = const std::vector<std::pair<std::string, std::string>>&;

    virtual ~IDatabase() = default;
    virtual void Connect() = 0;
    virtual void Disconnect() = 0;
    virtual void Execute(std::string_view query, SqlParams params) = 0;

    virtual std::vector<std::string> ExecuteQuery(std::string_view query, SqlParams params) = 0;
    template <typename Response, typename Func>
    Response Query(std::string_view query, SqlParams params, Func converter) {
        return converter(ExecuteQuery(query, params));
    }

    virtual void BeginTransaction() = 0;
    virtual void CommitTransaction() = 0;
    virtual void RollbackTransaction() = 0;
};