#pragma once


#include <thread>
#include <string>
#include <memory>


#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>


#include "random.h"
#include "handler.h"
#include "postgresql.h"
#include "TestConfig.h"


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;


namespace Request {
	inline http::request<http::string_body> CreateStandard(
		http::verb method,
		std::string_view target,
		json&& body) {

		http::request<http::string_body> req{ method, target, 11 };
		req.body() = body.dump(4);
		req.prepare_payload();

		return std::move(req);
	}


	inline http::request<http::string_body> CreateStandard(
		http::verb method,
		std::string_view target) {

		http::request<http::string_body> req{ method, target, 11 };
		req.prepare_payload();

		return std::move(req);
	}
}

class MiniSyncServer {
public:
	MiniSyncServer(net::io_context* ioc, const tcp::endpoint& endpoint)
		: m_ioc_ptr{ ioc }
		, m_acceptor{ *m_ioc_ptr, endpoint }
	{

	}

	template <typename FuncPtr>
	void Run(FuncPtr func) {
		tcp::socket socket{ *m_ioc_ptr };
		m_acceptor.accept(socket);
		DoSession(std::move(socket), func);
	}

	~MiniSyncServer() {
		m_acceptor.close();
	}

private:

	template <typename FuncPtr>
	void DoSession(tcp::socket&& socket, FuncPtr func) {
		beast::error_code ec;

		// This buffer is required to persist across reads
		beast::flat_buffer buffer;
		while (true)
		{
			// Read a request
			http::request<http::string_body> req;
			http::read(socket, buffer, req, ec);
			if (ec == http::error::end_of_stream) { break; }
			if (ec) { return; }

			http::message_generator msg = func -> operator()(std::move(req));

			// Determine if we should close the connection
			bool keep_alive = msg.keep_alive();

			// Send the response
			beast::write(socket, std::move(msg), ec);

			if (ec) { return; }
			if (!keep_alive) { break; }
		}

		socket.shutdown(tcp::socket::shutdown_send, ec);
	}


private:
	net::io_context* m_ioc_ptr;
	tcp::acceptor m_acceptor;
};

class Client {
public:
	Client(const tcp::endpoint& endpoint)
		: m_ioc{ 2 }
		, m_server{ &m_ioc, endpoint }
	{
	}

	template <typename FuncPtr>
	http::response<http::string_body> Query(http::request<http::string_body>&& req, FuncPtr func) {

		std::thread thread{ [&]() { m_server.Run(func); } };

		thread.detach();

		tcp::resolver resolver(m_ioc);
		beast::tcp_stream stream(m_ioc);

		auto const results = resolver.resolve("127.0.0.1", "8080");

		stream.connect(results);

		http::write(stream, req);

		beast::flat_buffer buffer;
		http::response<http::string_body> res;

		beast::error_code ec;

		http::read(stream, buffer, res, ec);
		if (ec) {

		}

		stream.socket().shutdown(tcp::socket::shutdown_both, ec);
		if (ec && ec != beast::errc::not_connected) {
			throw beast::system_error{ ec };
		}

		return std::move(res);
	}

private:
	net::io_context m_ioc;
	MiniSyncServer m_server;
};

class HttpHandlerTest : public testing::Test {
public:
	void SetUp() {

	}

	void TearDown() {

	}

	inline static PostgreSQL::ConnectionConfig config{ __HOST_DATABASE,
					  __USER_DATABASE, __PASSWORD_DATABASE, __NAME_DATABASE, __PORT_DATABASE };

	inline static std::shared_ptr<HttpHandler<http::string_body>> Handler{
		std::make_shared<HttpHandler<http::string_body>>(
				std::make_unique<PostgreSQL::Database>(config, std::make_shared<PostgreSQL::PGClient>()),
				"tests_handler",
				Random::StringGenerator()) };

	inline static Client Client{ 
		tcp::endpoint{
		net::ip::make_address("127.0.0.1"), 8080 } };
};