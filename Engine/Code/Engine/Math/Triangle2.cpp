#include "Triangle2.hpp"

Triangle2::Triangle2(Vec2 const& pointACounterClockwise, Vec2 const& pointBCounterClockwise, Vec2 const& pointCCounterClockwise)
{
	m_pointsCounterClockwise[0] = pointACounterClockwise;
	m_pointsCounterClockwise[1] = pointBCounterClockwise;
	m_pointsCounterClockwise[2] = pointCCounterClockwise;
}

Vec2 const Triangle2::GetCenterPos() const
{
	Vec2 center;
	center.x = (m_pointsCounterClockwise[0].x + m_pointsCounterClockwise[1].x + m_pointsCounterClockwise[2].x) / 3.f;
	center.y = (m_pointsCounterClockwise[0].y + m_pointsCounterClockwise[1].y + m_pointsCounterClockwise[2].y) / 3.f;
	return center;
}

void Triangle2::Translate(Vec2 const& translation)
{
	m_pointsCounterClockwise[0] += translation;
	m_pointsCounterClockwise[1] += translation;
	m_pointsCounterClockwise[2] += translation;
}

void Triangle2::RotateAboutCenter(float const& rotationDeltaDegrees)
{
	Vec2 center = GetCenterPos();
	Vec2 dispToA = m_pointsCounterClockwise[0] - center;
	Vec2 dispToB = m_pointsCounterClockwise[1] - center;
	Vec2 dispToC = m_pointsCounterClockwise[2] - center;

	dispToA.RotateDegrees(rotationDeltaDegrees);
	dispToB.RotateDegrees(rotationDeltaDegrees);
	dispToC.RotateDegrees(rotationDeltaDegrees);

	m_pointsCounterClockwise[0] = center + dispToA;
	m_pointsCounterClockwise[1] = center + dispToB;
	m_pointsCounterClockwise[2] = center + dispToC;
}



