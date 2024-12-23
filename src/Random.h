#pragma once

#include <random>

namespace Random
{
	static std::random_device dev;
	static std::mt19937 rng(dev());
	static std::uniform_int_distribution<std::mt19937::result_type> dist;

	static uint32_t UInt()
	{
		
		return dist(rng);
	}
}