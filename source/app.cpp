#include "server.h"
#include "handler.h"
#include "postgresql.h"
#include "random.h"
#include <iostream>

#include "Config.h"


int main() {
	std::cout << "url shortening service\n";

    auto const address = net::ip::make_address(__ADDRESS_SERVER);
    auto const port = static_cast<unsigned short>(__PORT_SERVER);

    Server<http::string_body> server{ address, port, 2 };

    PostgreSQL::ConnectionConfig config{ __HOST_DATABASE,
        __USER_DATABASE,
        __PASSWORD_DATABASE,
        __NAME_DATABASE,
        __PORT_DATABASE };

    std::unique_ptr<IDatabase> database{ std::make_unique<PostgreSQL::Database>(
        config, std::make_shared<PostgreSQL::PGClient>()) };

    auto handler = std::make_shared<HttpHandler<http::string_body>>(
        std::move(database), 
        "server_handler", 
        Random::StringGenerator());

    using RequestType = http::request<http::string_body, http::basic_fields<std::allocator<char>>>;
    
    auto func_lambda = [handler](auto&& req) ->  http::message_generator {
        return handler -> operator()(std::move(req));
    };

    std::function<http::message_generator(RequestType&&)> func = func_lambda;

    server.Run(func);

    return EXIT_SUCCESS;
}