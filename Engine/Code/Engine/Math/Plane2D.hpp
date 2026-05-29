#pragma once
#include "Engine/Math/Vec2.hpp"
struct Plane2D
{
public:
	Plane2D(Vec2 const& normal, float distance);
	Plane2D() {};
	~Plane2D() {};

	Vec2 GetNearestPointToOrigin() const;
	Vec2 GetNearestPoint(Vec2 const& referencePos) const;
	float GetAltitudeFromPoint(Vec2 const& referencePos) const;
	bool IsPointInFrontOf( Vec2 const& point) const;
	bool IsPointOnOrInFrontOf(Vec2 const& point) const;
	bool IsPointOnOrBehind(Vec2 const& point) const;
	void Translate(Vec2 const& translation);
	bool DoesPlaneIntersectWithPlane(Vec2& out_intersectionPoint, Plane2D const& otherPlane) const;
	bool DoesLineSegmentIntersectWithPlane(Vec2& out_intersectionPoint, Vec2 const& start, Vec2 const& end) const;
	Plane2D GetFlippedPlane() const;

	static bool ArePlanesEquivalent(Plane2D const& a, Plane2D const& b);

public:
	Vec2 m_normal = Vec2(1.f, 0.f);
	float m_distanceAlongNormal = 0.f;
};

