#pragma once
struct Vec2;
class RandomNumberGenerator
{
public:
	int RollRandomIntLessThan(int maxNotInclusive);
	int RollRandomIntInRange(int minInclusive, int maxInclusive);
	float RollRandomFloatZeroToOne();
	float RollRandomFloatInRange(float minInclusive, float maxInclusive);
	bool RollWithPercentChance(float percentSuccess);
	Vec2 RollRandomVec2DInRange(Vec2 const& minInclusive, Vec2 const& maxInclusive);

private:
	// to be added later
	// 
	//unsigned int m_seed = 0;
	//int m_position = 0;
};

