
#include <iostream>
#include <string>

#include "random_string_generator.h"


int main() {

	std::cout << "url shortening service\n";

	RandomStringGenerator generator{ };

	for (int i = 0; i < 10000; ++i) {
		std::string str{ generator.Generate() };
		std::cout << i << ") random_string_generator = " << str << "\n";
	}


	
	return 0;
}