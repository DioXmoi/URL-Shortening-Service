#pragma once

#include <gtest/gtest.h>

#include "random.h"


struct StringGeneratorTest : public testing::Test {

	void SetUp() {
		sg = new Random::StringGenerator{ 5, 10 };
	}

	void TearDown() {
		delete sg;
	}


	Random::StringGenerator* sg{ };
};