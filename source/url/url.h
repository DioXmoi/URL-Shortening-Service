#pragma once

#include <chrono>
#include <string>
#include <string_view>


using id = int;
using time_point_sys = std::chrono::time_point<std::chrono::system_clock>;

class uri {
public:

	uri(
		id id,
		std::string_view uri,
		std::string_view short_code,
		time_point_sys created_at = std::chrono::system_clock::now(),
		time_point_sys updated_at = std::chrono::system_clock::now(),
		int access_count = 0
	);
	
	// gets

	id get_id() const { return m_id; }
	std::string_view get_uri() const { return m_uri; }
	std::string_view get_short_code() const { return m_short_code; }
	time_point_sys get_created_at() const { return m_created_at; }
	time_point_sys get_updated_at() const { return m_updated_at; }
	int get_access_count() const { return m_access_count; }

	// sets

	void set_short_code(std::string_view code) {
		m_short_code = code;
		update();
	}

private:

	void update() { m_updated_at = std::chrono::system_clock::now(); }

private:
	id m_id{ };
	std::string m_uri{ };
	std::string m_short_code{ };
	time_point_sys m_created_at{ };
	time_point_sys m_updated_at{ };
	int m_access_count{ };
};
