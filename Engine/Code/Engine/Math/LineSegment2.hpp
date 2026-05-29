#pragma once
#include "Engine/Math/Vec2.hpp"
struct LineSegment2
{
public:
	Vec2 m_start;
	Vec2 m_end;

public:
	LineSegment2() {}
	~LineSegment2() {}
	explicit LineSegment2(Vec2 const& start, Vec2 const& end);
	explicit LineSegment2(Vec2 const& center, Vec2 directionNormalized, float const& length);

	//Accessors
	Vec2 const GetCenterPos() const;

	//Mutators
	void Translate(Vec2 const& translation);
	void SetCenter(Vec2 const& newCenter);
	void RotateAboutCenter(float const& rotationDeltaDegrees);
};

