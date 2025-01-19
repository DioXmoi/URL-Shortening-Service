#pragma once

#include <chrono>
#include <string>
#include <string_view>


using Id = int;
using TimePointSys = std::chrono::time_point<std::chrono::system_clock>;

class Uri {
public:

	Uri(
		Id Id,
		std::string_view Uri,
		std::string_view short_code,
		TimePointSys created_at = std::chrono::system_clock::now(),
		TimePointSys updated_at = std::chrono::system_clock::now(),
		int access_count = 0
	);
	
	// gets

	Id GetId() const { return m_id; }
	std::string_view GetUri() const { return m_uri; }
	std::string_view GetShortCode() const { return m_shortCode; }
	TimePointSys GetCreatedAt() const { return m_createdAt; }
	TimePointSys GetUpdatedAt() const { return m_updatedAt; }
	int GetAccessCount() const { return m_accessCount; }

	// sets

	void SetShortCode(std::string_view code) {
		m_shortCode = code;
		Update();
	}

private:

	void Update() { m_updatedAt = std::chrono::system_clock::now(); }

private:
	Id m_id{ };
	std::string m_uri{ };
	std::string m_shortCode{ };
	TimePointSys m_createdAt{ };
	TimePointSys m_updatedAt{ };
	int m_accessCount{ };
};
