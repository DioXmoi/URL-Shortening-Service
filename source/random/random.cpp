#include "random.h"

std::mt19937 random::generate() {
	std::random_device rd{ };

	// Create seed_seq with clock and 7 random numbers from std::random_device
	std::seed_seq ss{
		static_cast<std::seed_seq::result_type>(std::chrono::steady_clock::now().time_since_epoch().count()),
			rd(), rd(), rd(), rd(), rd(), rd(), rd() };

	return std::mt19937{ ss };
}