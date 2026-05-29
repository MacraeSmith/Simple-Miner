#pragma once
#include "Engine/Math/Vec3.hpp"
struct ZSphere3D
{
public: 
	Vec3 m_center;
	float m_radius = 1.f;

	ZSphere3D() {}
	explicit ZSphere3D(Vec3 const& center, float radius);
	~ZSphere3D() {}

	void Translate(Vec3 const& translation);
};

