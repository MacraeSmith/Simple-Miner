#include "Engine/Math/ConvexHull2.hpp"
#include "Engine/Math/ConvexPoly2.hpp"
#include "Engine/Math/MathUtils.hpp"


ConvexHull2::ConvexHull2(std::vector<Plane2D> planes)
	:m_planes(planes)
{
}

ConvexHull2::ConvexHull2(ConvexPoly2 const& convexPoly)
{
	MakeFromConvexPoly(convexPoly);
}

void ConvexHull2::Translate(Vec2 const& translation)
{
	for (int i = 0; i < (int)m_planes.size(); ++i)
	{
		m_planes[i].Translate(translation);
	}
}

bool ConvexHull2::IsPointInside(Vec2 const& point) const
{
	for (int i = 0; i < (int)m_planes.size(); ++i)
	{
		if (m_planes[i].GetAltitudeFromPoint(point) > 0.f)
		{
			return false;
		}
	}

	return true;
}

void ConvexHull2::MakeFromConvexPoly(ConvexPoly2 const& convexPoly)
{
	m_planes.clear();
	const int numPlanes = convexPoly.GetNumVertexPositions();
	m_planes.reserve(numPlanes);

	std::vector<Vec2> vertexPositions = convexPoly.GetVertexPositions();

	for (int i = 0; i < numPlanes; ++i)
	{
		Vec2 a = vertexPositions[i];
		int j = i + 1 < numPlanes ? i + 1 : 0;
		Vec2 b = vertexPositions[j];

		Vec2 ab = (b - a);
		Vec2 abNormalized = ab.GetNormalized();

		Vec2 planeNormal = abNormalized.GetRotatedMinus90Degrees();
		float planeDistance = DotProduct2D(a, planeNormal);

		Plane2D plane(planeNormal, planeDistance);
		m_planes.push_back(plane);
	}
}
