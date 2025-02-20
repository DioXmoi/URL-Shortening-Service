#include <gtest/gtest.h>

#include "TestStringGenerator.h"



TEST_F(StringGeneratorTest, ConstructorThrowsIfLenFromIsZero) {
    ASSERT_THROW(Random::StringGenerator(0, 10), std::invalid_argument);
}

TEST_F(StringGeneratorTest, ConstructorThrowsIfLenToIsZero) {
    ASSERT_THROW(Random::StringGenerator(5, 0), std::invalid_argument);
}

TEST_F(StringGeneratorTest, ConstructorThrowsIfCharsIsEmpty) {
    std::vector<char> emptyChars{ };
    ASSERT_THROW(Random::StringGenerator(5, 10, emptyChars), std::invalid_argument);
}

TEST_F(StringGeneratorTest, ConstructorInitializesCorrectly) {
    std::string str{ sg -> Generate() };

    EXPECT_GE(str.size(), 5);
    EXPECT_LE(str.size(), 10);
}

TEST_F(StringGeneratorTest, GenerateStringContainsOnlyValidChars) {
    std::string str{ sg -> Generate() };

    for (char ch : str) {
        EXPECT_NE(std::find(
            sg -> baseChars.begin(),
            sg -> baseChars.end(), ch),
            sg -> baseChars.end());
    }
}

TEST_F(StringGeneratorTest, GenerateStringWithCustomChars) {
    std::vector<char> customChars = { 'A', 'B', 'C' };
    Random::StringGenerator generator{ 5, 10, customChars };

    std::string str{ generator.Generate() };

    for (char ch : str) {
        EXPECT_NE(std::find(
            customChars.begin(), 
            customChars.end(), ch), 
            customChars.end());
    }
}