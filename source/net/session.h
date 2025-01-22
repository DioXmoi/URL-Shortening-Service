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

class Session : std::enable_shared_from_this<Session> {

public:
	// Take ownership of the stream
	Session(tcp::socket&& socket) noexcept
		: m_stream{ std::move(socket) }
	{

	}


	void Run() {
		net::dispatch(m_stream.get_executor(),
			beast::bind_front_handler(
				&Session::DoRead,
				shared_from_this()));
	}

	void DoRead() {
		m_req = { };

		m_stream.expires_after(std::chrono::seconds(30));

		http::async_read(m_stream, m_buffer, m_req,
			beast::bind_front_handler(
				&Session::OnRead,
				shared_from_this()));
	}


	void OnRead(beast::error_code ec, std::size_t bytes_transferred) {
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
			HandleRequest(std::move(m_req)));
	}

	void SendResponse(http::message_generator&& msg) {
		bool keep_alive = msg.keep_alive();

		// Write the response
		beast::async_write(
			m_stream,
			std::move(msg),
			beast::bind_front_handler(
				&Session::OnWrite, shared_from_this(), keep_alive));
	}

	void
		OnWrite(
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


	void DoClose() {
		// Send a TCP shutdown
		beast::error_code ec;
		m_stream.socket().shutdown(tcp::socket::shutdown_send, ec);

		// At this point the connection is closed gracefully
	}


	template <class Body, class Allocator>
	http::message_generator HandleRequest(
		http::request<Body, http::basic_fields<Allocator>>&& req)
	{
		http::response<http::string_body> res{ http::status::bad_request, req.version() };
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = std::string("BiB BUB");
		res.prepare_payload();
		return res;
	}


private:
	beast::flat_buffer m_buffer{ };
	beast::tcp_stream m_stream{ };
	http::request<http::string_body> m_req{ };
};