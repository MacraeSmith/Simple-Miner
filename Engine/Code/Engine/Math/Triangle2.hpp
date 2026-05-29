#pragma once
#include "Engine/Math/Vec2.hpp"
struct Triangle2
{
public:
	Vec2 m_pointsCounterClockwise[3] = {};

public:
	Triangle2() {}
	~Triangle2() {}
	explicit Triangle2(Vec2 const& pointACounterClockwise, Vec2 const& pointBCounterClockwise, Vec2 const& pointCCounterClockwise);

	//Accessors
	Vec2 const GetCenterPos() const;

	//Mutators
	void Translate(Vec2 const& translation);
	void RotateAboutCenter(float const& rotationDeltaDegrees);
};

