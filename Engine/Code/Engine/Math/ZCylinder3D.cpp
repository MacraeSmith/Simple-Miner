#include "Engine/Math/ZCylinder3D.hpp"
#include "Engine/Math/Vec3.hpp"

ZCylinder3D::ZCylinder3D(Vec2 const& centerXY, FloatRange const& zRange, float radius)
	:m_centerXY(centerXY)
	,m_zRange(zRange)
	,m_radius(radius)
{
}

ZCylinder3D::ZCylinder3D(Vec3 const& bottom, float height, float radius)
	:m_centerXY(Vec2(bottom.x, bottom.y))
	,m_zRange(FloatRange(bottom.z, bottom.z + height))
	,m_radius(radius)
{
}

Vec3 ZCylinder3D::GetCenter() const
{
	return Vec3(m_centerXY.x, m_centerXY.y, m_zRange.m_min + (.5f * (m_zRange.m_max - m_zRange.m_min)));
}

void ZCylinder3D::SetCenter(Vec3 const& newCenter)
{
	m_centerXY = Vec2(newCenter.x, newCenter.y);
	float halfLength = .5f * (m_zRange.m_max - m_zRange.m_min);
	m_zRange = FloatRange(newCenter.z - halfLength, newCenter.z + halfLength);
}

float ZCylinder3D::GetHeight() const
{
	return m_zRange.m_max - m_zRange.m_min;
}
Vec3 ZCylinder3D::GetBottom() const
{
	return Vec3(m_centerXY, m_zRange.m_min);
}
