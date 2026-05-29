#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/LineSegment2.hpp"

Capsule2::Capsule2(Vec2 const& start, Vec2 const& end, float const& radius)
	:m_start(start)
	,m_end(end)
	,m_radius(radius)
{
}

Capsule2::Capsule2(LineSegment2 const& bone, float const& radius)
	:m_start(bone.m_start)
	,m_end(bone.m_end)
	,m_radius(radius)
{
}

LineSegment2 const Capsule2::GetBone() const
{
	return LineSegment2(m_start, m_end);
}

Vec2 const Capsule2::GetCenterPos() const
{
	return (m_start + m_end) * 0.5f;
}

float Capsule2::GetCapsuleLength() const
{
	return (m_end - m_start).GetLength() + m_radius + m_radius;
}

void Capsule2::Translate(Vec2 const& translation)
{
	m_start += translation;
	m_end += translation;
}

void Capsule2::SetCenter(Vec2 const& newCenter)
{
	Vec2 displacement = m_end - m_start;
	float distanceOffset = displacement.GetLength() * 0.5f;
	displacement.SetLength(distanceOffset);

	m_end = newCenter + displacement;
	m_start = newCenter - displacement;
}

void Capsule2::RotateAboutCenter(float const& rotationDeltaDegrees)
{
	Vec2 center = (m_start + m_end) * 0.5f;

	Vec2 displacement = m_end - center;
	displacement.RotateDegrees(rotationDeltaDegrees);

	m_end = center + displacement;
	m_start = center - displacement;
}
