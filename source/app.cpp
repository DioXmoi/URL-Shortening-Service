#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <server.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


http::message_generator Handler(http::request<http::string_body> req) {
    http::response<http::string_body> res{ http::status::internal_server_error, req.version() };
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: ' Bib bub'";
    res.prepare_payload();
    return res;
}

int main() {

	std::cout << "url shortening service\n";

    auto const address = net::ip::make_address("127.0.0.0");
    auto const port = static_cast<unsigned short>(80);

    Server<http::string_body> server{ address, port  };

    std::function<http::message_generator(http::request<http::string_body>)> func{ Handler };
    server.Run(func);

    return EXIT_SUCCESS;
}