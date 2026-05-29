#include "Engine/Math/HashUtils.hpp"

uint64_t SplitMix64_Next(uint64_t& state)
{
	state = (state + 0x9E3779B97F4A7C15ULL);
	uint64_t z = state;
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	z = (z ^ (z >> 31));
	return z;
}

float U64ToUnitFloat01(uint64_t v)
{
	// Use top 24 bits for stable float in [0,1).
	uint32_t mantissa24 = (uint32_t)(v >> 40); // top 24 bits
	return ((float)mantissa24) * 0.000000059604644775390625f; // 1 / 16777216
}

float GetDeterministicRandom01(uint32_t seed)
{
	seed ^= 2747636419u;
	seed *= 2654435769u;
	seed ^= (seed >> 16);
	seed *= 2654435769u;
	seed ^= (seed >> 16);
	seed *= 2654435769u;

	return (float)(seed & 0x00FFFFFF) / (float)0x01000000;
}
