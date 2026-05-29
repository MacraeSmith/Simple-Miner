#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/Vec2.hpp"
#include <stdlib.h>


int RandomNumberGenerator::RollRandomIntLessThan(int maxNotInclusive)
{
	return rand() % maxNotInclusive;
}

int RandomNumberGenerator::RollRandomIntInRange(int minInclusive, int maxInclusive)
{
	int range = (maxInclusive - minInclusive) + 1;
	return minInclusive + RollRandomIntLessThan(range);
}

float RandomNumberGenerator::RollRandomFloatZeroToOne()
{
	return (float)(rand() / (float)(32767));
}

float RandomNumberGenerator::RollRandomFloatInRange(float minInclusive, float maxInclusive)
{
	return (minInclusive + (maxInclusive - minInclusive) * RollRandomFloatZeroToOne());
}

bool RandomNumberGenerator::RollWithPercentChance(float percentSuccess)
{
	return percentSuccess >= RollRandomFloatZeroToOne();
}

Vec2 RandomNumberGenerator::RollRandomVec2DInRange(Vec2 const& minInclusive, Vec2 const& maxInclusive)
{
	float xPos = RollRandomFloatInRange(minInclusive.x, maxInclusive.x);
	float yPos = RollRandomFloatInRange(minInclusive.y, maxInclusive.y);
	return Vec2(xPos, yPos);
}


