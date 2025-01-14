#include "RandomStringGenerator.h"

RandomStringGenerator::RandomStringGenerator(int len_from, int len_to, std::vector<char> chars)
    : m_from{ len_from }
    , m_to{ len_to }
    , m_chars{ base_chars }
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

std::string RandomStringGenerator::generate() {
    std::string sequence{ };

    std::size_t len{ static_cast<std::size_t>(random::get(m_from, m_to)) };
    sequence.reserve(len);

    int min{ 0 };
    int max{ static_cast<int>(m_chars.size() - 1) };
    for (std::size_t i{ 0 }; i < len; ++i) {
        sequence += m_chars[static_cast<std::size_t>(random::get(min, max))];
    }

    return sequence;
}