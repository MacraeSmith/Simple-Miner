#include "Engine/Core/Vertex_PCU.hpp"

Vertex_PCU::Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords)
	:m_position (position)
	,m_color (color)
	,m_uvTexCoords (uvTexCoords)
{
}

Vertex_PCU::Vertex_PCU(Vec2 const& position2D, Rgba8 const& color, Vec2 const& uvTexCoords)
	:m_position(Vec3(position2D.x, position2D.y, 0.f))
	, m_color(color)
	, m_uvTexCoords(uvTexCoords)
{
}


