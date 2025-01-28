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


#include "spdlog/spdlog.h"
#include "spdlog/async.h" //поддержка асинхронного ведения журнала.
#include "spdlog/sinks/basic_file_sink.h"


#include "session.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


template <class Body, class Allocator = http::basic_fields<std::allocator<char>>>
using HandlerPtr = std::shared_ptr<std::function<http::message_generator(http::request<Body, Allocator>)>>;

using LoggerPtr = std::shared_ptr<spdlog::logger>;

// Accepts incoming connections and launches the sessions
template <class Body, class Allocator = http::basic_fields<std::allocator<char>>>
class Listener : public std::enable_shared_from_this<Listener<Body, Allocator>> {
public:
    Listener(net::io_context& ioc, 
        tcp::endpoint endpoint, 
        HandlerPtr<Body, Allocator> handler, 
        LoggerPtr logger);

    // Start avoccepting incoming connections
    void Run();

private:

    void DoAccept();

    void OnAccept(beast::error_code ec, tcp::socket socket);

private:
    net::io_context& m_ioc;
    tcp::acceptor m_acceptor;

    HandlerPtr<Body, Allocator> m_handler;
    LoggerPtr m_logger;
};


template <class Body, class Allocator>
Listener<Body, Allocator>::Listener(
    net::io_context& ioc,
    tcp::endpoint endpoint,
    HandlerPtr<Body, Allocator> handler,
    LoggerPtr logger)
    : m_ioc(ioc)
    , m_acceptor(net::make_strand(ioc))
    , m_handler{ handler }
    , m_logger{ logger }
{
    beast::error_code ec;

    // Open the acceptor
    m_acceptor.open(endpoint.protocol(), ec);
    if (ec) {
        m_logger -> error(ec.what());
        throw std::exception(ec.what().c_str());
    }

    // Allow address reuse
    m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        m_logger -> error(ec.what());
        throw std::exception(ec.what().c_str());
    }

    // Bind to the server address
    m_acceptor.bind(endpoint, ec);
    if (ec) {
        m_logger -> error(ec.what());
        throw std::exception(ec.what().c_str());
    }

    // Start listening for connections
    m_acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        m_logger -> error(ec.what());
        throw std::exception(ec.what().c_str());
    }
}


template <class Body, class Allocator>
void Listener<Body, Allocator>::Run() {
    DoAccept();
}


template <class Body, class Allocator>
void Listener<Body, Allocator>::DoAccept() {
    // The new connection gets its own strand
    m_acceptor.async_accept(
        net::make_strand(m_ioc),
        beast::bind_front_handler(
            &Listener<Body, Allocator>::OnAccept,
            std::enable_shared_from_this<Listener<Body, Allocator>>::shared_from_this()));
}

template <class Body, class Allocator>
void Listener<Body, Allocator>::OnAccept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        m_logger -> error(ec.what());
        return; // To avoid infinite loop
    }
    else {
        // Create the session and Run it

        auto ptr = std::make_shared<Session<Body, Allocator>>(std::move(socket), m_handler, m_logger);

        ptr -> Run();
    }

    // Accept another connection
    DoAccept();
}