#include "Engine/Math/Plane3D.hpp"
#include "Engine/Math/MathUtils.hpp"

Plane3D::Plane3D(Vec3 const& normal, float distance)
	:m_normal(normal)
	,m_distanceAlongNormal(distance)
{
}

Plane3D::Plane3D(Vec3 const& a, Vec3 const& b, Vec3 const& c)
{
	m_normal = CrossProduct3D(b - a, c - a).GetNormalized();
	m_distanceAlongNormal = DotProduct3D(m_normal, a);
}

Vec3 Plane3D::GetNearestPointToOrigin() const
{
	return m_normal * m_distanceAlongNormal;
}

Vec3 Plane3D::GetNearestPoint(Vec3 const& referencePos) const
{
	float altitude = GetAltitudeFromPoint(referencePos);
	return referencePos - (m_normal * altitude);
}

float Plane3D::GetAltitudeFromPoint(Vec3 const& referencePos) const
{
	return DotProduct3D(m_normal, referencePos) - m_distanceAlongNormal;
}

bool Plane3D::IsPointInFrontOf(Vec3 const& point) const
{
	float alitude = GetAltitudeFromPoint(point);
	return alitude >= 0.f;
}
