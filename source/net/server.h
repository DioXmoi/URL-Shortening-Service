#pragma once


#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <stdexcept>


#include "spdlog/spdlog.h"
#include "spdlog/async.h" //поддержка асинхронного ведения журнала.
#include "spdlog/sinks/basic_file_sink.h"

#include "listener.h"
#include "session.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


using LoggerPtr = std::shared_ptr<spdlog::logger>;

template <class Body, class Allocator = http::basic_fields<std::allocator<char>>>
class Server {
public:
    Server(net::ip::address address, unsigned short port, int countThreads = 1);

    void Run(const std::function<http::message_generator(
        http::request<Body, Allocator>)>& handler);

    void Stop();

    ~Server();

private:

    net::ip::address m_address{ };
    unsigned short m_port{ };
    int m_countThreads{ };
    net::io_context m_ioc{ };
    std::vector<std::thread> m_threads{ };
    std::shared_ptr<Listener<Body, Allocator>> m_listener{ };

    LoggerPtr m_logger{ };
};



template <class Body, class Allocator>
Server<Body, Allocator>::Server(net::ip::address address, unsigned short port, int countThreads)
    : m_address{ address }
    , m_port{ port }
    , m_countThreads{ countThreads }
    , m_ioc{ countThreads }
{
    if (countThreads < 1) {
        throw std::invalid_argument("The number of threads cannot be less than one");
    }

    m_logger = spdlog::get("ServerBibBub");
    if (!m_logger) {
        m_logger = spdlog::basic_logger_mt<spdlog::async_factory>("ServerBibBub", "logs/server.txt");
    }
}



template <class Body, class Allocator>
void Server<Body, Allocator>::Run(const std::function<http::message_generator(
    http::request<Body, Allocator>)>& handler) {
    try {
        m_logger -> info("Server starting on {}:{}", m_address.to_string(), m_port);

        auto endpoint{ net::ip::tcp::endpoint{ m_address, m_port } };
        auto funcPtr{ std::make_shared<std::function<http::message_generator(http::request<Body, Allocator>)>>(handler) };

        m_listener = std::make_shared<Listener<Body, Allocator>>(
            m_ioc, 
            endpoint,
            funcPtr,
            m_logger);


        for (auto threadCounter{ m_countThreads - 1 }; threadCounter > 0; --threadCounter) {
            m_threads.emplace_back(std::bind(&net::io_context::run, &m_ioc));
        }

        m_listener -> Run();

        m_ioc.run();
    }
    catch (const std::exception& ex) {
        m_logger -> error("Exception during server start: {}", ex.what());
    }
}

template <class Body, class Allocator>
void Server<Body, Allocator>::Stop() {
    m_logger -> info("Server stopping");
    m_ioc.stop();
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

}


template <class Body, class Allocator>
Server<Body, Allocator>::~Server() {
    spdlog::drop("ServerBibBub");
    Stop();
}