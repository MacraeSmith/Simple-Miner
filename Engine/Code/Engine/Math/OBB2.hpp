#pragma once
#include "Engine/Math/Vec2.hpp"
struct OBB2
{
public:
	Vec2 m_center;
	Vec2 m_iBasisNormal;
	Vec2 m_halfDimensionsIJ;

public:
	OBB2() {}
	~OBB2() {}
	explicit OBB2(Vec2 const& center, Vec2 const& iBasisNormal, Vec2 const& halfDimensionsIJ);

	void const GetCornerPoints(Vec2* out_forCornerWorldPositions) const;
	Vec2 const GetLocalPosForWorldPos(Vec2 const& worldPos) const;
	Vec2 const GetWorldPosForLocalPos(Vec2 const& localPos) const;
	bool const IsPointInside(Vec2 const& point) const;

	//Mutators
	void RotateAboutCenter(float rotationDeltaDegrees);
	void Translate(Vec2 const& translation);
};

