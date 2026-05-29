#pragma once
#include "Engine/Math/Vec3.hpp"
#include <vector>
struct AABB2;
struct Vec2;
class RandomNumberGenerator;
struct AABB3
{
public:
	Vec3 m_mins;
	Vec3 m_maxs;

public:
	AABB3() {}
	~AABB3() {}
	AABB3(AABB3 const& copyFrom);
	explicit AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
	explicit AABB3(Vec3 const& mins, Vec3 const& maxs);
	explicit AABB3(AABB2 const& xAndYBounds, Vec2 const& zBounds);

	bool IsPointOnOrInside(Vec3 const& point) const;
	bool IsPointInsideBounds(Vec3 const& point) const;
	Vec3 const GetCenterPos() const;
	Vec3 const GetDimensions() const;
	Vec3 const GetNearestPoint(Vec3 const& referencePos) const;
	
	void Translate(Vec3 const& translation);
	void SetCenter(Vec3 const& newCenter);
	void SetDimensions(Vec3 const& newDimensions);
	void StretchToIncludePoint(Vec3 const& point);

	Vec3 GetRandomPointInside(RandomNumberGenerator* rng = nullptr);

};

