#include "Engine/Core/Vertex_PCUTBN.hpp"

Vertex_PCUTBN::Vertex_PCUTBN(Vec3 const& position, Rgba8 const& color, Vec3 const& normal, Vec2 uvTexCoords)
	:m_position(position)
	,m_color(color)
	,m_normal(normal)
	,m_uvTexCoords(uvTexCoords)
{
}

Vertex_PCUTBN::Vertex_PCUTBN(Vec3 const& position, Rgba8 const& color, Vec3 const& tangent, Vec3 const& biTangent, Vec3 const& normal, Vec2 uvTexCoords)
	:m_position(position)
	,m_color(color)
	,m_tangent(tangent)
	,m_biTangent(biTangent)
	,m_normal(normal)
	,m_uvTexCoords(uvTexCoords)
{
}
