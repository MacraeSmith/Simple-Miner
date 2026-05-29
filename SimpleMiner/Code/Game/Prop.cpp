#include "Game/Prop.hpp"
#include "Game/GameCommon.hpp"

#include "Engine/Renderer/RendererDX11.hpp"

Prop::Prop(Game* owner)
	:Entity(owner)
{
}

Prop::Prop(Game* owner, std::vector<Vertex_PCU> const& vertexes, Vec3 const& position)
	:Entity(owner, position)
	,m_vertexes(vertexes)
{
}

void Prop::Update(float deltaSeconds)
{
	m_orientation += m_angularVelocity * deltaSeconds;
}

void Prop::Render() const
{
	g_renderer->SetModelConstants(GetModelToWorldTransform(), m_color);
	g_renderer->SetBlendMode(BlendMode::OPAQUE);
	g_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
	g_renderer->BindTexture(m_texture);
	g_renderer->DrawVertexArray(m_vertexes);
}
