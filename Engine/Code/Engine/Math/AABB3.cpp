#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

AABB3::AABB3(AABB3 const& copyFrom)
	:m_mins(copyFrom.m_mins)
	,m_maxs(copyFrom.m_maxs)
{
}

AABB3::AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
	:m_mins(Vec3(minX, minY, minZ))
	,m_maxs(Vec3(maxX, maxY, maxZ))
{
}

AABB3::AABB3(Vec3 const& mins, Vec3 const& maxs)
	:m_mins(mins)
	,m_maxs(maxs)
{
}

AABB3::AABB3(AABB2 const& xAndYBounds, Vec2 const& zBounds)
	:m_mins(Vec3(xAndYBounds.m_mins.x, xAndYBounds.m_mins.y, zBounds.x))
	,m_maxs(Vec3(xAndYBounds.m_maxs.x, xAndYBounds.m_maxs.y, zBounds.y))
{
}

bool AABB3::IsPointOnOrInside(Vec3 const& point) const
{
	return point.x >= m_mins.x && point.x <= m_maxs.x
		&& point.y >= m_mins.y && point.y <= m_maxs.y
		&& point.z >= m_mins.z && point.z <= m_maxs.z;
}

bool AABB3::IsPointInsideBounds(Vec3 const& point) const
{
	return point.x > m_mins.x && point.x < m_maxs.x
		&& point.y > m_mins.y && point.y < m_maxs.y
		&& point.z > m_mins.z && point.z < m_maxs.z;
}

Vec3 const AABB3::GetCenterPos() const
{
	return (m_mins + m_maxs) * 0.5f;
}

Vec3 const AABB3::GetDimensions() const
{
	return m_maxs - m_mins;
}

Vec3 const AABB3::GetNearestPoint(Vec3 const& referencePos) const
{
	float nearestX = GetClamped(referencePos.x, m_mins.x, m_maxs.x);
	float nearestY = GetClamped(referencePos.y, m_mins.y, m_maxs.y);
	float nearestZ = GetClamped(referencePos.z, m_mins.z, m_maxs.z);
	return Vec3(nearestX, nearestY, nearestZ);
}


void AABB3::Translate(Vec3 const& translation)
{
	m_mins += translation;
	m_maxs += translation;
}

void AABB3::SetCenter(Vec3 const& newCenter)
{
	Vec3 offset = newCenter - GetCenterPos();
	m_mins += offset;
	m_maxs += offset;
}

void AABB3::SetDimensions(Vec3 const& newDimensions)
{
	Vec3 center = GetCenterPos();
	float xOffset = newDimensions.x * 0.5f;
	float yOffset = newDimensions.y * 0.5f;
	float zOffset = newDimensions.z * 0.5f;
	m_mins = Vec3(center.x - xOffset, center.y - yOffset, center.z - zOffset);
	m_maxs = Vec3(center.x + xOffset, center.y + yOffset, center.z + zOffset);
}

void AABB3::StretchToIncludePoint(Vec3 const& point)
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

	if (m_mins.z > point.z)
	{
		m_mins.z = point.z;
	}

	if (m_maxs.z < point.z)
	{
		m_maxs.z = point.z;
	}
}

Vec3 AABB3::GetRandomPointInside(RandomNumberGenerator* rng)
{
	float x = rng->RollRandomFloatInRange(m_mins.x, m_maxs.x);
	float y = rng->RollRandomFloatInRange(m_mins.y, m_maxs.y);
	float z = rng->RollRandomFloatInRange(m_mins.z, m_maxs.z);
	return Vec3(x,y,z);
}
