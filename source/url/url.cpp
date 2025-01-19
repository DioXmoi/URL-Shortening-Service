#include "url.h"


Uri::Uri(
	Id id,
	std::string_view uri,
	std::string_view shortCode,
	TimePointSys createdAt,
	TimePointSys updatedAt,
	int accessCount
)
	: m_id{ id }
	, m_shortCode{ shortCode }
	, m_createdAt{ createdAt }
	, m_updatedAt{ updatedAt }
	, m_accessCount{ accessCount }
{
}