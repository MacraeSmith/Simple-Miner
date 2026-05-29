#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/MathUtils.hpp"

Disc2::Disc2(Vec2 const& center, float const& radius)
	:m_center(center)
	,m_radius(radius)
{
}

Disc2 Disc2::GetDiscFromTwoPoints(Vec2 const& a, Vec2 const& b)
{
	Disc2 disc;
	disc.m_center = (a + b) * 0.5f;
	disc.m_radius = GetDistance2D(a, b) * 0.5f;
	return disc;
}

Disc2 Disc2::GetDiscFromThreePoints(Vec2 const& a, Vec2 const& b, Vec2 const& c)
{
	float ax = a.x, ay = a.y;
	float bx = b.x, by = b.y;
	float cx = c.x, cy = c.y;

	float d = 2.0f * (ax * (by - cy) +
		bx * (cy - ay) +
		cx * (ay - by));

	// Collinear guard
	if (fabsf(d) < 1e-6f)
	{
		Disc2 ab = Disc2::GetDiscFromTwoPoints(a, b);
		Disc2 ac = Disc2::GetDiscFromTwoPoints(a, c);
		Disc2 bc = Disc2::GetDiscFromTwoPoints(b, c);

		Disc2 result = ab;
		if (ac.m_radius > result.m_radius) result = ac;
		if (bc.m_radius > result.m_radius) result = bc;
		return result;
	}

	float ax2ay2 = ax * ax + ay * ay;
	float bx2by2 = bx * bx + by * by;
	float cx2cy2 = cx * cx + cy * cy;

	Vec2 center;
	center.x = (ax2ay2 * (by - cy) +
		bx2by2 * (cy - ay) +
		cx2cy2 * (ay - by)) / d;

	center.y = (ax2ay2 * (cx - bx) +
		bx2by2 * (ax - cx) +
		cx2cy2 * (bx - ax)) / d;

	Disc2 disc;
	disc.m_center = center;
	disc.m_radius = GetDistance2D(center, a);
	return disc;
}

bool Disc2::IsPointInside(Vec2 const& point) const
{
	Vec2 disp = point - m_center;
	return disp.GetLengthSquared() < (m_radius * m_radius);
}

void Disc2::Translate(Vec2 const& translation)
{
	m_center += translation;
}
