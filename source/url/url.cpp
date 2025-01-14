#include "url.h"


uri::uri(
	id id,
	std::string_view uri,
	std::string_view short_code,
	time_point_sys created_at,
	time_point_sys updated_at,
	int access_count
)
	: m_id{ id }
	, m_short_code{ short_code }
	, m_created_at{ created_at }
	, m_updated_at{ updated_at }
	, m_access_count{ access_count }
{
}