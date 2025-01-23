#include "listener.h"



void Listener::DoAccept() {
    // The new connection gets its own strand
    m_acceptor.async_accept(
        net::make_strand(m_ioc),
        beast::bind_front_handler(
            &Listener::OnAccept,
            shared_from_this()));
}

void Listener::OnAccept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        // fail(ec, "accept");
        return; // To avoid infinite loop
    }
    else {
        // Create the session and Run it
        std::make_shared<Session>(std::move(socket))->Run();
    }

    // Accept another connection
    DoAccept();
}

Listener::Listener(
    net::io_context& ioc,
    tcp::endpoint endpoint)
    : m_ioc(ioc)
    , m_acceptor(net::make_strand(ioc))
{
    beast::error_code ec;

    // Open the acceptor
    m_acceptor.open(endpoint.protocol(), ec);
    if (ec) {
        // fail(ec, "open");
        return;
    }

    // Allow address reuse
    m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        // fail(ec, "set_option");
        return;
    }

    // Bind to the server address
    m_acceptor.bind(endpoint, ec);
    if (ec) {
        // fail(ec, "bind");
        return;
    }

    // Start listening for connections
    m_acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        // fail(ec, "listen");
        return;
    }
}

// Start accepting incoming connections
void Listener::Run() {
    DoAccept();
}