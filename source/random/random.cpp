#include <stdexcept>
#include <utility>


#include "random.h"

std::mt19937 Random::Generate() {
	std::random_device rd{ };

	// Create seed_seq with clock and 7 Random numbers from std::random_device
	std::seed_seq ss{
		static_cast<std::seed_seq::result_type>(std::chrono::steady_clock::now().time_since_epoch().count()),
			rd(), rd(), rd(), rd(), rd(), rd(), rd() };

	return std::mt19937{ ss };
}


Random::StringGenerator::StringGenerator(int len_from, int len_to, std::vector<char> chars)
    : m_from{ len_from }
    , m_to{ len_to }
    , m_chars{ baseChars }
{
    if (m_from <= 0 || m_to <= 0) {
        throw std::invalid_argument("The length of the parameters cannot be zero or negative.");
    }

    if (chars.empty()) {
        throw std::invalid_argument("The character array cannot be empty.");
    }

    if (m_to < m_from) {
        std::swap(m_from, m_to);
    }
}


std::string Random::StringGenerator::Generate() {
    std::string sequence{ };

    std::size_t len{ static_cast<std::size_t>(Random::Get(m_from, m_to)) };
    sequence.reserve(len);

    int min{ 0 };
    int max{ static_cast<int>(m_chars.size() - 1) };
    for (std::size_t i{ 0 }; i < len; ++i) {
        sequence += m_chars[static_cast<std::size_t>(Random::Get(min, max))];
    }

    return sequence;
}