#include "server.h"

#include "postgresql.h"

#include "spdlog/spdlog.h"
#include "spdlog/async.h" //поддержка асинхронного ведения журнала.
#include "spdlog/sinks/basic_file_sink.h"


http::message_generator Handler(http::request<http::string_body> req);

int main() {

    PostgreSQL db{ };

    auto ec = boost::system::error_code{ };
    db.connect(ec);
    if (ec) {
        std::cout << "That`s is BAD......" << ec.what() << "\n";
        return EXIT_FAILURE;
    }

    try {
        auto async_file = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", "logs/async_log.txt");  
        for (int i = 1; i < 101; ++i) { 
            async_file -> info("Async message #{}", i); 
        }  // Under VisualStudio, this must be called before main finishes to workaround a known VS issue
        spdlog::drop_all();
    }
    catch (const spdlog::spdlog_ex& ex) { 
        std::cout << "Log initialization failed: " << ex.what() << std::endl; 
    } 

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