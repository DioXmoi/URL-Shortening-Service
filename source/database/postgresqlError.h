#include <string>
#include <stdexcept>


namespace PostgreSQLError {
	class PostgreSQLError : public std::runtime_error {
	public:
		PostgreSQLError(const std::string& msg)
			: std::runtime_error{ msg }
		{
		}

		PostgreSQLError(const char* msg)
			: std::runtime_error{ msg }
		{
		}
	};


	class ExecuteError : public PostgreSQLError {
	public:
		ExecuteError(const std::string& msg)
			: PostgreSQLError{ msg }
		{
		}

		ExecuteError(const char* msg)
			: PostgreSQLError{ msg }
		{
		}
	};


	class ConnectionConfigError : public PostgreSQLError {
	public:
		ConnectionConfigError(const std::string& msg)
			: PostgreSQLError{ msg }
		{
		}

		ConnectionConfigError(const char* msg)
			: PostgreSQLError{ msg }
		{
		}
	};


	class ConnectionPoolError : public PostgreSQLError {
	public:
		ConnectionPoolError(const std::string& msg)
			: PostgreSQLError{ msg }
		{
		}

		ConnectionPoolError(const char* msg)
			: PostgreSQLError{ msg }
		{
		}
	};


	class ConnectError : public ConnectionPoolError {
	public:
		ConnectError(const std::string& msg)
			: ConnectionPoolError{ msg }
		{
		}

		ConnectError(const char* msg)
			: ConnectionPoolError{ msg }
		{
		}
	};


	class ResetError : public ConnectError {
	public:
		ResetError(const std::string& msg)
			: ConnectError{ msg }
		{
		}

		ResetError(const char* msg)
			: ConnectError{ msg }
		{
		}
	};
}