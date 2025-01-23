#include "session.h"


Session::Session(tcp::socket&& socket) noexcept
	: m_stream{ std::move(socket) }
{

}


void Session::Run() {
	net::dispatch(m_stream.get_executor(),
		beast::bind_front_handler(
			&Session::DoRead,
			shared_from_this()));
}

void Session::DoRead() {
	m_req = { };

	m_stream.expires_after(std::chrono::seconds(30));

	http::async_read(m_stream, m_buffer, m_req,
		beast::bind_front_handler(
			&Session::OnRead,
			shared_from_this()));
}


void Session::OnRead(beast::error_code ec, std::size_t bytes_transferred) {
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

void Session::SendResponse(http::message_generator&& msg) {
	bool keep_alive = msg.keep_alive();

	// Write the response
	beast::async_write(
		m_stream,
		std::move(msg),
		beast::bind_front_handler(
			&Session::OnWrite, shared_from_this(), keep_alive));
}

void Session::OnWrite(
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


void Session::DoClose() {
	// Send a TCP shutdown
	beast::error_code ec;
	m_stream.socket().shutdown(tcp::socket::shutdown_send, ec);

	// At this point the connection is closed gracefully
}