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


#include "listener.h"
#include "session.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


template <class Body, class Allocator = http::basic_fields<std::allocator<char>>>
class Server {
public:
    Server(net::ip::address address, unsigned short port, int threads = 1);

    void Run(const std::function<http::message_generator(
        http::request<Body, Allocator>)>& handler);

private:

    net::ip::address m_address{ };
    unsigned short m_port{ };
    int m_threads{ };
    net::io_context m_ioc{ };

    std::vector<std::thread> m_data{ };
    std::shared_ptr<Listener<Body, Allocator>> m_listener{ };
};



template <class Body, class Allocator>
Server<Body, Allocator>::Server(net::ip::address address, unsigned short port, int threads)
    : m_address{ address }
    , m_port{ port }
    , m_threads{ threads }
    , m_ioc{ threads }
{
    if (threads < 1) {
        throw std::invalid_argument("The number of threads cannot be less than one");
    }
}



template <class Body, class Allocator>
void Server<Body, Allocator>::Run(const std::function<http::message_generator(
    http::request<Body, Allocator>)>& handler) {
    for (auto i{ m_threads - 1 }; i > 0; --i) {
        m_data.emplace_back(
            [&m_ioc]() {
                m_ioc.run();
            });
    }

    m_listener = std::make_shared<Listener<Body, Allocator>>(m_ioc, { m_address, m_port }, handler);

    m_ioc.run();
}