#include "Engine/Math/Plane2D.hpp"
#include "Engine/Math/MathUtils.hpp"

Plane2D::Plane2D(Vec2 const& normal, float distance)
	:m_normal(normal)
	, m_distanceAlongNormal(distance)
{
}

Vec2 Plane2D::GetNearestPointToOrigin() const
{
	return m_normal * m_distanceAlongNormal;
}

Vec2 Plane2D::GetNearestPoint(Vec2 const& referencePos) const
{
	float altitude = GetAltitudeFromPoint(referencePos);
	return referencePos - (m_normal * altitude);
}

float Plane2D::GetAltitudeFromPoint(Vec2 const& referencePos) const
{
	return DotProduct2D(m_normal, referencePos) - m_distanceAlongNormal;
}

bool Plane2D::IsPointInFrontOf(Vec2 const& point) const
{
	float alitude = GetAltitudeFromPoint(point);
	return alitude >= 0.f;
}

bool Plane2D::IsPointOnOrInFrontOf(Vec2 const& point) const
{
	float alitude = GetAltitudeFromPoint(point);
	return alitude >= 0.f;
}

bool Plane2D::IsPointOnOrBehind(Vec2 const& point) const
{
	float alitude = GetAltitudeFromPoint(point);
	return alitude < 0.f;
}

void Plane2D::Translate(Vec2 const& translation)
{
	m_distanceAlongNormal += DotProduct2D(translation, m_normal);
}

bool Plane2D::DoesPlaneIntersectWithPlane(Vec2& out_intersectionPoint, Plane2D const& otherPlane) const
{
	float n1x = m_normal.x;
	float n1y = m_normal.y;
	float n2x = otherPlane.m_normal.x;
	float n2y = otherPlane.m_normal.y;

	float d1 = m_distanceAlongNormal;
	float d2 = otherPlane.m_distanceAlongNormal;

	float det = (n1x * n2y) - (n1y * n2x);
	float eps = 1e-6f;
	if ((det > -eps) && (det < eps))
	{
		return false;
	}

	float invDet = (1.0f / det);
	float x = (((d1 * n2y) - (n1y * d2)) * invDet);
	float y = (((n1x * d2) - (d1 * n2x)) * invDet);

	out_intersectionPoint = Vec2(x, y);
	return true;
}

bool Plane2D::DoesLineSegmentIntersectWithPlane(Vec2& out_intersectionPoint, Vec2 const& start, Vec2 const& end) const
{
	float aStart = GetAltitudeFromPoint(start);
	float aEnd = GetAltitudeFromPoint(end);


	if (aStart * aEnd >= 0.f)
	{
		return false;
	}

	float denom = (aStart - aEnd);

	float t = (aStart / denom);

	out_intersectionPoint = Lerp(start, end, t);
	
	return true;
}

Plane2D Plane2D::GetFlippedPlane() const
{
	return Plane2D(-m_normal, -m_distanceAlongNormal);
}

bool Plane2D::ArePlanesEquivalent(Plane2D const& a, Plane2D const& b)
{
	// Same plane
	if (a.m_normal == b.m_normal && a.m_distanceAlongNormal == b.m_distanceAlongNormal)
	{
		return true;
	}

	// Same geometric plane but flipped
	if (a.m_normal == -b.m_normal && a.m_distanceAlongNormal == -b.m_distanceAlongNormal)
	{
		return true;
	}

	return false;
}
