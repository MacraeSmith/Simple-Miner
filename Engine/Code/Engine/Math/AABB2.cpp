#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

AABB2 const AABB2::ZERO = AABB2(0.f, 0.f, 0.f, 0.f);
AABB2 const AABB2::ONE = AABB2(1.f, 1.f, 1.f, 1.f);
AABB2 const AABB2::ZERO_TO_ONE = AABB2(0.f, 0.f, 1.f, 1.f);
AABB2 const AABB2::ONE_TO_ZERO = AABB2(1.f, 1.f, 0.f, 0.f);

AABB2::AABB2(AABB2 const& copyFrom)
	:m_mins(copyFrom.m_mins)
	,m_maxs(copyFrom.m_maxs)
{
}

AABB2::AABB2(float minX, float minY, float maxX, float maxY)
	:m_mins(Vec2(minX, minY))
	,m_maxs(Vec2(maxX, maxY))
{
}

AABB2::AABB2(Vec2 const& mins, Vec2 const& maxs)
	:m_mins(mins)
	,m_maxs(maxs)
{
}

bool AABB2::IsPointOnOrInside(Vec2 const& point) const
{
	return point.x >= m_mins.x && point.x <= m_maxs.x
		&& point.y >= m_mins.y && point.y <= m_maxs.y;
}

bool AABB2::IsPointInsideBounds(Vec2 const& point) const
{
	return point.x > m_mins.x && point.x < m_maxs.x
		&& point.y > m_mins.y && point.y < m_maxs.y;
}

bool AABB2::IsDiscInside(Vec2 const& discCenter, float discRadius) const
{
	return discCenter.x - discRadius >= m_mins.x && discCenter.x + discRadius <= m_maxs.x
		&& discCenter.y - discRadius >= m_mins.y && discCenter.y + discRadius <= m_maxs.y;
}


Vec2 const AABB2::GetCenterPos() const
{
	return (m_mins + m_maxs) * 0.5f;
}

Vec2 const AABB2::GetDimensions() const
{
	return m_maxs - m_mins;
}

float AABB2::GetHeight() const
{
	return m_maxs.y - m_mins.y;
}

float AABB2::GetWidth() const
{
	return m_maxs.x - m_mins.x;
}

float AABB2::GetAspect() const
{
	Vec2 dimensions = GetDimensions();
	return dimensions.x / dimensions.y;
}

Vec2 const AABB2::GetNearestPoint(Vec2 const& referencePosition) const
{
	float nearestX = GetClamped(referencePosition.x, m_mins.x, m_maxs.x);
	float nearestY = GetClamped(referencePosition.y, m_mins.y, m_maxs.y);
	return Vec2(nearestX, nearestY);
}

Vec2 const AABB2::GetPointAtUV(Vec2 const& uv) const
{
	float newX = RangeMap(uv.x,0.f, 1.f, m_mins.x, m_maxs.x);
	float newY = RangeMap(uv.y, 0.f, 1.f, m_mins.y, m_maxs.y);
	return Vec2(newX, newY);
}

Vec2 const AABB2::GetUVForPoint(Vec2 const& point) const
{
	float newX = RangeMap(point.x, m_mins.x, m_maxs.x, 0.f, 1.f);
	float newY = RangeMap(point.y, m_mins.y, m_maxs.y, 0.f, 1.f);
	return Vec2(newX, newY);
}

Vec2 const AABB2::GetRandomPointInBounds(RandomNumberGenerator* randomNumberGenerator)
{
	RandomNumberGenerator rng;
	if (randomNumberGenerator)
	{
		rng = *randomNumberGenerator;
	}
	float xPos = rng.RollRandomFloatInRange(m_mins.x, m_maxs.x);
	float yPos = rng.RollRandomFloatInRange(m_mins.y, m_maxs.y);
	
	return Vec2(xPos, yPos);
}

Vec2 const AABB2::GetRandomPointOnEdgeOfBounds(RandomNumberGenerator* randomNumberGenerator)
{
	Vec2 randPos;
	RandomNumberGenerator rng;
	if (randomNumberGenerator)
	{
		rng = *randomNumberGenerator;
	}

	int side = rng.RollRandomIntInRange(0, 3);
	
	switch (side)
	{
	case 0: // bottom
		randPos.x = rng.RollRandomFloatInRange(m_mins.x, m_maxs.x);
		randPos.y = m_mins.y;
		break;
	case 1: //right
		randPos.x = m_maxs.x;
		randPos.y = rng.RollRandomFloatInRange(m_mins.y, m_maxs.y);
		break;
	case 2: //top
		randPos.x = rng.RollRandomFloatInRange(m_mins.x, m_maxs.x);
		randPos.y = m_maxs.y;
		break;
	case 3: //left
		randPos.x = m_mins.x;
		randPos.y = rng.RollRandomFloatInRange(m_mins.y, m_maxs.y);
		break;
	default:
		randPos.x = rng.RollRandomFloatInRange(m_mins.x, m_maxs.x);
		randPos.y = m_mins.y;
		break;
	}
	return randPos;
}

AABB2 const AABB2::GetBoxAtUVS(Vec2 const& uvMins, Vec2 const& uvMaxs) const
{
	return AABB2(GetPointAtUV(uvMins), GetPointAtUV(uvMaxs));
}

AABB2 const AABB2::GetBoxAtUVS(AABB2 const& uvs) const
{
	return AABB2(GetPointAtUV(uvs.m_mins), GetPointAtUV(uvs.m_maxs));
}

void AABB2::Translate(Vec2 const& translationToApply)
{
	m_mins += translationToApply;
	m_maxs += translationToApply;
}

void AABB2::TranslateX(float translationX)
{
	m_mins.x += translationX;
	m_maxs.x += translationX;
}

void AABB2::TranslateY(float translationY)
{
	m_mins.y += translationY;
	m_maxs.y += translationY;
}

void AABB2::SetCenter(Vec2 const& newCenter)
{
	Vec2 offset = newCenter - GetCenterPos();
	m_maxs += offset;
	m_mins += offset;
}

void AABB2::SetDimensions(Vec2 const& newDimensions)
{
	Vec2 center = GetCenterPos();
	float xOffset = newDimensions.x * 0.5f;
	float yOffset = newDimensions.y * 0.5f;
	m_mins = Vec2(center.x - xOffset, center.y - yOffset);
	m_maxs = Vec2(center.x + xOffset, center.y + yOffset);
}

void AABB2::StretchToIncludePoint(Vec2 const& point)
{
	if (m_mins.x > point.x)
	{
		m_mins.x = point.x;
	}

	if (m_maxs.x < point.x)
	{
		m_maxs.x = point.x;
	}

	if (m_mins.y > point.y)
	{
		m_mins.y = point.y;
	}

	if (m_maxs.y < point.y)
	{
		m_maxs.y = point.y;
	}
}

void AABB2::AddPadding(float xPadPercent, float yPadPadPercent)
{
	Vec2 dims = GetDimensions();
	m_mins.x -= (xPadPercent * dims.x);
	m_mins.y -= (yPadPadPercent * dims.y);
	m_maxs.x += (xPadPercent * dims.x);
	m_maxs.y += (yPadPadPercent * dims.y);
}

void AABB2::AddPadding(float minXPadPercent, float minYPadPercent, float maxXPadPercent, float maxYPadPercent)
{

	Vec2 dims = GetDimensions();
	m_mins.x -= (minXPadPercent * dims.x);
	m_mins.y -= (minYPadPercent * dims.y);
	m_maxs.x += (maxXPadPercent * dims.x);
	m_maxs.y += (maxYPadPercent * dims.y);
}

void AABB2::AddPadding(Vec2 const& minsPadPercent, Vec2 const& maxsPadPercent)
{
	Vec2 dims = GetDimensions();
	m_mins.x -= (minsPadPercent.x * dims.x);
	m_mins.y -= (minsPadPercent.y * dims.y);
	m_maxs.x += (maxsPadPercent.x * dims.x);
	m_maxs.y += (maxsPadPercent.y * dims.y);
}

std::vector<AABB2> AABB2::GetHorizontalSlicedBoxesTopToBottom(int numBoxes)
{
	std::vector<AABB2> slicedBoxes;
	slicedBoxes.reserve(numBoxes);

	float verticalStep = GetDimensions().y / (float)numBoxes;
	for (int i = 0; i < numBoxes; ++i)
	{
		float yMax = m_maxs.y - (i * verticalStep);
		float yMin = m_maxs.y - ((i + 1) * verticalStep);
		slicedBoxes.push_back(AABB2(m_mins.x, yMin, m_maxs.x, yMax));
	}

	return slicedBoxes;
}

std::vector<AABB2> AABB2::GetVerticalSlicedBoxesLeftToRight(int numBoxes)
{
	std::vector<AABB2> slicedBoxes;
	slicedBoxes.reserve(numBoxes);

	float horizontalStep = GetDimensions().x / (float)numBoxes;
	for (int i = 0; i < numBoxes; ++i)
	{
		float xMin = m_mins.x + horizontalStep * i;
		float xMax = m_mins.x + horizontalStep * (i + 1);
		slicedBoxes.push_back(AABB2(xMin, m_mins.y, xMax, m_maxs.y));
	}

	return slicedBoxes;
}

AABB2 AABB2::ChopOffTop(float percentOfOriginialToChop)
{
	float maxY = m_maxs.y;
	AddPadding(Vec2::ZERO, Vec2(0.f, -percentOfOriginialToChop));
	return AABB2(m_mins.x, m_mins.y, m_maxs.y, maxY);
}

AABB2 AABB2::ChopOffBottom(float percentToChopOff)
{
	float minsY = m_mins.y;
	AddPadding(Vec2(0.f, -percentToChopOff), Vec2::ZERO);
	return AABB2(m_mins.x, minsY, m_maxs.x, m_mins.y);
}

AABB2 AABB2::ChopOffLeft(float percentToChopOff)
{
	float minsX = m_mins.x;
	AddPadding(Vec2( -percentToChopOff, 0.f), Vec2::ZERO);
	return AABB2(minsX, m_mins.y, m_mins.x, m_maxs.y);
}

AABB2 AABB2::ChopOffRight(float percentToChopOff)
{
	float maxsX = m_maxs.x;
	AddPadding(Vec2::ZERO, Vec2(-percentToChopOff, 0.f));
	return AABB2(m_maxs.x, maxsX, m_mins.y, m_maxs.y);
}

bool AABB2::operator==(AABB2 otherBox)
{
	return m_mins == otherBox.m_mins && m_maxs == otherBox.m_maxs;
}

bool AABB2::operator!=(AABB2 otherBox)
{
	return m_mins != otherBox.m_mins || m_maxs != otherBox.m_maxs;
}

// void AABB2::AddPadding(Vec2 const& xPadPercent_MinMax, Vec2 const& yPadPercent_MinMax)
// {
// 	Vec2 dims = GetDimensions();
// 	m_mins.x += (xPadPercent_MinMax.x * dims.x);
// 	m_mins.y += (yPadPercent_MinMax.x * dims.y);
// 	m_maxs.x -= (xPadPercent_MinMax.y * dims.x);
// 	m_maxs.y -= (yPadPercent_MinMax.y * dims.y);
// }

