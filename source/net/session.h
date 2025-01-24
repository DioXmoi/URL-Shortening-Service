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


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>



template <class Body, class Allocator = http::basic_fields<std::allocator<char>>>
using HandlerPtr = std::shared_ptr<std::function<http::message_generator(http::request<Body, Allocator>)>>;

template <class Body, class Allocator = http::basic_fields<std::allocator<char>>>
class Session : std::enable_shared_from_this<Session<Body, Allocator>> {

public:
	// Take ownership of the stream
	Session(tcp::socket&& socket, HandlerPtr<Body, Allocator> handler);


	void Run();

	void DoRead();

	void OnRead(beast::error_code ec, std::size_t bytes_transferred);

	void SendResponse(http::message_generator&& msg);

	void OnWrite(bool keep_alive, beast::error_code ec,
		std::size_t bytes_transferred);

	void DoClose();

private:
	beast::flat_buffer m_buffer{ };
	beast::tcp_stream m_stream;
	http::request<http::string_body> m_req{ };
	HandlerPtr<Body, Allocator> m_handler{ };
};


template <class Body, class Allocator>
Session<Body, Allocator>::Session(tcp::socket&& socket, HandlerPtr<Body, Allocator> handler)
	: m_stream{ std::move(socket) }
	, m_handler{ handler }
{

}

template <class Body, class Allocator>
void Session<Body, Allocator>::Run() {
	net::dispatch(m_stream.get_executor(),
		beast::bind_front_handler(
			&Session<Body, Allocator>::DoRead,
			shared_from_this<Body, Allocator>()));
}

template <class Body, class Allocator>
void Session<Body, Allocator>::DoRead() {
	m_req = { };

	m_stream.expires_after(std::chrono::seconds(30));

	http::async_read(m_stream, m_buffer, m_req,
		beast::bind_front_handler(
			&Session<Body, Allocator>::OnRead,
			shared_from_this<Body, Allocator>()));
}


template <class Body, class Allocator>
void Session<Body, Allocator>::OnRead(beast::error_code ec, std::size_t bytes_transferred) {
	boost::ignore_unused(bytes_transferred);

	// This means they closed the connection
	if (ec == http::error::end_of_stream) {
		return DoClose();
	}

	if (ec) {
		//return fail(ec, "read");
	}

	// Send the response
	SendResponse(
		m_handler(std::move(m_req)));
}


template <class Body, class Allocator>
void Session<Body, Allocator>::SendResponse(http::message_generator&& msg) {
	bool keep_alive = msg.keep_alive();
	// Write the response
	beast::async_write(
		m_stream,
		std::move(msg),
		beast::bind_front_handler(
			&Session<Body, Allocator>::OnWrite, shared_from_this<Body, Allocator>(), keep_alive));
}


template <class Body, class Allocator>
void Session<Body, Allocator>::OnWrite(
	bool keep_alive,
	beast::error_code ec,
	std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec) {
		// return fail(ec, "write");
	}

	if (!keep_alive)
	{
		// This means we should close the connection, usually because
		// the response indicated the "Connection: close" semantic.
		return DoClose();
	}

	// Read another request
	DoRead();
}


template <class Body, class Allocator>
void Session<Body, Allocator>::DoClose() {
	// Send a TCP shutdown
	beast::error_code ec;
	m_stream.socket().shutdown(tcp::socket::shutdown_send, ec);

	// At this point the connection is closed gracefully
}