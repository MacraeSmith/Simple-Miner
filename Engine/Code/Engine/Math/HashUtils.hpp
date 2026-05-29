#pragma once
#include <cstdint>

uint64_t SplitMix64_Next(uint64_t& state);
float U64ToUnitFloat01(uint64_t v);
float GetDeterministicRandom01(uint32_t seed);