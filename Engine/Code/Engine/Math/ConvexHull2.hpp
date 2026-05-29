#pragma once
#include "Engine/Math/Plane2D.hpp"
#include <vector>
struct Vec2;
struct ConvexPoly2;
struct ConvexHull2
{
public:
	ConvexHull2() {};
	explicit ConvexHull2(std::vector<Plane2D> planes);
	explicit ConvexHull2(ConvexPoly2 const& convexPoly);
	~ConvexHull2() {};

	void Translate(Vec2 const& translation);
	bool IsPointInside(Vec2 const& point) const;
	
	void MakeFromConvexPoly(ConvexPoly2 const& convexPoly);

public:
	std::vector<Plane2D> m_planes;
};

