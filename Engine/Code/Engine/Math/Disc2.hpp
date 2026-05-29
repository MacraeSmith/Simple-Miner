#pragma once
#include "Engine/Math/Vec2.hpp"
struct Disc2
{
public:
	Vec2 m_center;
	float m_radius = 1.f;

public:
	Disc2() {}
	~Disc2() {}
	explicit Disc2(Vec2 const& center, float const& radius);

	static Disc2 GetDiscFromTwoPoints(Vec2 const& a, Vec2 const& b);
	static Disc2 GetDiscFromThreePoints(Vec2 const& a, Vec2 const& b, Vec2 const& c);

	//Accessors
	bool IsPointInside(Vec2 const& point) const;

	//Mutators
	void Translate(Vec2 const& translation);
};

