#pragma once
#include "Engine/Math/Vec2.hpp"
struct LineSegment2;
struct Capsule2
{
public:
	Vec2 m_start;
	Vec2 m_end;
	float m_radius = 0.f;

public:
	Capsule2(){}
	~Capsule2() {}
	explicit Capsule2(Vec2 const& start, Vec2 const& end, float const& radius);
	explicit Capsule2(LineSegment2 const& bone, float const& radius);

	//Accessors
	LineSegment2 const GetBone() const;
	Vec2 const GetCenterPos() const;
	float GetCapsuleLength() const;

	//Mutators
	void Translate(Vec2 const& translation);
	void SetCenter(Vec2 const& newCenter);
	void RotateAboutCenter(float const& rotationDeltaDegrees);

	
};

