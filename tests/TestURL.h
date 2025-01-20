#pragma once

#include <gtest/gtest.h>

#include "url.h"

struct UrlTest : public testing::Test {
	void SetUp() { 
		url = new Url(0, "UrlTest.text", "test"); 
	}

	void TearDown() { 
		delete url; 
	}

	Url* url{ };
};