#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"

LineSegment2::LineSegment2(Vec2 const& start, Vec2 const& end)
	:m_start(start)
	,m_end(end)
{
}

LineSegment2::LineSegment2(Vec2 const& center, Vec2 directionNormalized, float const& length)
{
	directionNormalized.Normalize();
	directionNormalized.SetLength(length * 0.5f);
	m_start = center - directionNormalized;
	m_end = center + directionNormalized;
}

Vec2 const LineSegment2::GetCenterPos() const
{
	return (m_start + m_end) * 0.5f;
}

void LineSegment2::Translate(Vec2 const& translation)
{
	m_start += translation;
	m_end += translation;
}

void LineSegment2::SetCenter(Vec2 const& newCenter)
{
	Vec2 displacement = m_end - m_start;
	float distanceOffset = displacement.GetLength() * 0.5f;
	displacement.SetLength(distanceOffset);

	m_end = newCenter + displacement;
	m_start = newCenter - displacement;
}

void LineSegment2::RotateAboutCenter(float const& rotationDeltaDegrees)
{
	Vec2 center = (m_start + m_end) * 0.5f;

	Vec2 displacement = m_end - center;
	displacement.RotateDegrees(rotationDeltaDegrees);

	m_end = center + displacement;
	m_start = center - displacement;

}

