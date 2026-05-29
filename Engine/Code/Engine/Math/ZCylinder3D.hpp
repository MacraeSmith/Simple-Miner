#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/FloatRange.hpp"
struct Vec3;
struct ZCylinder3D
{
public:
	Vec2 m_centerXY;
	FloatRange m_zRange;
	float m_radius = 0.f;

	ZCylinder3D() {}
	explicit ZCylinder3D(Vec2 const& centerXY, FloatRange const& zRange, float radius);
	explicit ZCylinder3D(Vec3 const& bottom, float height, float radius);
	~ZCylinder3D() {}

	Vec3 GetCenter() const;
	void SetCenter(Vec3 const& newCenter);
	float GetHeight() const;
	Vec3 GetBottom() const;
};

