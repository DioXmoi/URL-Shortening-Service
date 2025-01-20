#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <stdexcept>


using Id = int;
using TimePointSys = std::chrono::time_point<std::chrono::system_clock>;

class Url {
public:

	Url(
		Id id,
		std::string_view url,
		std::string_view short_code,
		TimePointSys created_at = std::chrono::system_clock::now(),
		TimePointSys updated_at = std::chrono::system_clock::now(),
		int access_count = 0
	);
	
	// gets

	Id GetId() const { return m_id; }
	std::string_view GetUri() const { return m_url; }
	std::string_view GetShortCode() const { return m_shortCode; }
	TimePointSys GetCreatedAt() const { return m_createdAt; }
	TimePointSys GetUpdatedAt() const { return m_updatedAt; }
	int GetAccessCount() const { return m_accessCount; }

	// sets

	void SetShortCode(std::string_view code) {
		if (code.empty()) {
			throw std::invalid_argument("Short code cannot be empty\n");
		}

		m_shortCode = code;
		Update();
	}

	void IncrementAccessCount() { 
		++m_accessCount; 
		Update();
	}

private:

	void Update() { m_updatedAt = std::chrono::system_clock::now(); }

private:
	Id m_id{ };
	std::string m_url{ };
	std::string m_shortCode{ };
	TimePointSys m_createdAt{ };
	TimePointSys m_updatedAt{ };
	int m_accessCount{ };
};
