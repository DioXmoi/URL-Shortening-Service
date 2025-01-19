#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <utility>

#include "random.h"

class RandomStringGenerator {
public:
    inline static const std::vector<char> baseChars{
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
    };

    RandomStringGenerator(int len_from = 4, 
        int len_to = 16, 
        std::vector<char> chars = RandomStringGenerator::baseChars);

    std::string Generate();


private:
    int m_from{ };
    int m_to{ };
    std::vector<char> m_chars{ };
};