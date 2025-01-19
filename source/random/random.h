#pragma once

#include <chrono>
#include <random>


namespace Random {
	// Returns a seeded Mersenne Twister
	// Note: we'd prefer to return a std::seed_seq (to initialize a std::mt19937), but std::seed can't be copied, so it can't be returned by value.
	// Instead, we'll create a std::mt19937, seed it, and then return the std::mt19937 (which can be copied).
	std::mt19937 Generate();

	// Here's our global std::mt19937 object.
	// The inline keyword means we only have one global instance for our whole program.
	inline std::mt19937 mt{ Generate() }; // generates a seeded std::mt19937 and copies it into our global object

	// Generate a Random int between [min, max] (inclusive)
	inline int Get(int min, int max) {
		return std::uniform_int_distribution{ min, max }(mt);
	}

	// Generate a Random value between [min, max] (inclusive)
	// * min and max have same type
	// * Return value has same type as min and max
	// * Supported types:
	// *    short, int, long, long long
	// *    unsigned short, unsigned int, unsigned long, or unsigned long long
	// Sample call: Random::Get(1L, 6L);             // returns long
	// Sample call: Random::Get(1u, 6u);             // returns unsigned int
	template <typename T>
	T Get(T min, T max) {
		return std::uniform_int_distribution<T>{ min, max }(mt);
	}

	// Generate a Random value between [min, max] (inclusive)
	// * min and max can have different types
	// * Must explicitly specify return type as template type argument
	// * min and max will be converted to the return type
	// Sample call: Random::Get<std::size_t>(0, 6);  // returns std::size_t
	// Sample call: Random::Get<std::size_t>(0, 6u); // returns std::size_t
	// Sample call: Random::Get<std::int>(0, 6u);    // returns int
	template <typename R, typename S, typename T>
	R Get(S min, T max) {
		return Get<R>(static_cast<R>(min), static_cast<R>(max));
	}
}