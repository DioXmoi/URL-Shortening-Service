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
	Session(tcp::socket&& socket) noexcept;


	void Run();

	void DoRead();

	void OnRead(beast::error_code ec, std::size_t bytes_transferred);

	void SendResponse(http::message_generator&& msg);

	void OnWrite(bool keep_alive, beast::error_code ec,
		std::size_t bytes_transferred);

	void DoClose();

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
	beast::tcp_stream m_stream;
	http::request<http::string_body> m_req{ };
};