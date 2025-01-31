#include "server.h"

#include "postgresql.h"

#include "spdlog/spdlog.h"
#include "spdlog/async.h" //поддержка асинхронного ведения журнала.
#include "spdlog/sinks/basic_file_sink.h"


http::message_generator Handler(http::request<http::string_body> req);

int main() {

	std::cout << "url shortening service\n";
    auto const address = net::ip::make_address("127.0.0.1");
    auto const port = static_cast<unsigned short>(80);

    Server<http::string_body> server{ address, port  };

    std::function<http::message_generator(http::request<http::string_body>)> func{ Handler };
    server.Run(func);

    return EXIT_SUCCESS;
}


http::message_generator Handler(http::request<http::string_body> req) {
    http::response<http::string_body> res{ http::status::internal_server_error, req.version() };
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: ' Bib bub'";
    res.prepare_payload();
    return res;
}