#pragma once
#include"Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"

struct Vertex_PCU
{
public:
	Vec3 m_position;
	Rgba8 m_color;
	Vec2 m_uvTexCoords;
public:
	~Vertex_PCU(){}
	Vertex_PCU(){}

	explicit Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords = Vec2(0.f, 0.f));
	explicit Vertex_PCU(Vec2 const& position2D, Rgba8 const& color, Vec2 const& uvTexCoords = Vec2(0.f, 0.f));

};

