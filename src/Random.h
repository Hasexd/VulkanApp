#pragma once

#include <random>

namespace Random
{
	static uint32_t UInt()
	{
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<std::mt19937::result_type> dist;

		return dist(rng);
	}
}