#include "url.h"


Url::Url(
	Id id,
	std::string_view url,
	std::string_view shortCode,
	TimePointSys createdAt,
	TimePointSys updatedAt,
	int accessCount
)
	: m_id{ id }
	, m_url{ url }
	, m_shortCode{ shortCode }
	, m_createdAt{ createdAt }
	, m_updatedAt{ updatedAt }
	, m_accessCount{ accessCount }
{
}