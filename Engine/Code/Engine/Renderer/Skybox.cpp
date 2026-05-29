#include "Engine/Renderer/Skybox.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/DebugRender.hpp"

#ifdef RENDERER_DX12
	#include "Engine/Renderer/RendererDX12.hpp"
	#include "Engine/Renderer/TextureDX12.hpp"
	#include "Engine/Renderer/ShaderDX12.hpp"
	#include "Engine/Renderer/VertexBufferDX12.hpp"
	#include "Engine/Renderer/IndexBufferDX12.hpp"
#else
	#include "Engine/Renderer/RendererDX11.hpp"
	#include "Engine/Renderer/Texture.hpp"
	#include "Engine/Renderer/Shader.hpp"
	#include "Engine/Renderer/VertexBuffer.hpp"
	#include "Engine/Renderer/IndexBuffer.hpp"
#endif // RENDERER_DX12

#ifdef RENDERER_DX12
Skybox::Skybox(RendererDX12* renderer)
	:m_renderer(renderer)
{
}
Skybox::Skybox(RendererDX12* renderer, std::vector<TextureDX12 const*> textures)
	:m_renderer(renderer)
	,m_textures(textures)
{
	AABB3 bounds(Vec3::ONE * -0.5f, Vec3::ONE * 0.5f);
	Verts verts;
	IndexList indexes;
	AddVertsForSkybox(verts, indexes, bounds);
	m_vbo = m_renderer->CreateVertexBuffer(verts, "Skybox_VBO");
	m_ibo = m_renderer->CreateIndexBuffer(indexes, "Skybox_IBO");
}
#else
Skybox::Skybox(RendererDX11* renderer)
	:m_renderer(renderer)
{
}
Skybox::Skybox(RendererDX11* renderer, std::vector<Texture const*> textures)
	:m_renderer(renderer)
	, m_textures(textures)
{
	AABB3 bounds(Vec3::ONE * -0.5f, Vec3::ONE * 0.5f);
	Verts verts;
	IndexList indexes;
	AddVertsForInvertedIndexedAABB3D(verts, indexes, bounds);
	m_vbo = m_renderer->CreateVertexBuffer((unsigned int)(verts.size() * sizeof(Vertex_PCU)), sizeof(Vertex_PCU));
	m_ibo = m_renderer->CreateIndexBuffer((unsigned int)(indexes.size() * sizeof(unsigned int)));
	m_renderer->CopyCPUToGPU(verts.data(), (unsigned int)(verts.size() * sizeof(Vertex_PCU)), m_vbo);
	m_renderer->CopyCPUToGPU(indexes.data(), (unsigned int)(indexes.size() * sizeof(unsigned int)), m_ibo);
}

#endif

Skybox::~Skybox()
{
	
	for (int i = 0; i < (int)m_textures.size(); ++i)
	{
		m_textures[i] = nullptr;
	}

	delete m_vbo;
	m_vbo = nullptr;

	delete m_ibo;
	m_ibo = nullptr;

	m_shader = nullptr;
	
}

void Skybox::UpdatePosition(Vec3 const& position)
{
	m_position = position;
}

void Skybox::Render() const
{
	for (int i = 0; i < (int)m_textures.size(); ++i)
	{
		m_renderer->BindTexture(m_textures[i], i);
	}

	m_renderer->BindShader(m_shader);
	m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_FRONT);
	m_renderer->SetDepthMode(DepthMode::DISABLED);
	m_renderer->SetBlendMode(BlendMode::OPAQUE);
	m_renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);

	Mat44 transform;
	transform.SetTranslation3D(m_position);
	transform.AppendScaleUniform3D(m_universalScale);
	m_renderer->SetModelConstants(transform);

	m_renderer->DrawIndexedVertexBuffer(m_vbo, m_ibo, m_ibo->GetNumIndexes());
	m_renderer->ClearDepth();
}

#ifdef RENDERER_DX12

SkyboxSphere::SkyboxSphere(RendererDX12* renderer, TextureDX12 const* texture, int numSlices)
	:Skybox(renderer)
{
	m_textures.push_back(texture);

	Verts verts;
	IndexList indexes;
	AddVertsForInvertedIndexedZSphere3D(verts, indexes, Vec3::ZERO, 10.f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, numSlices, numSlices / 2);
	m_vbo = m_renderer->CreateVertexBuffer(verts, "SkyboxSphere_VBO");
	m_ibo = m_renderer->CreateIndexBuffer(indexes, "SkyboxSphere_IBO");
}



#else

SkyboxSphere::SkyboxSphere(RendererDX11* renderer, Texture const* texture, int numSlices)
	:Skybox(renderer)
{
	m_textures.push_back(texture);

	Verts verts;
	IndexList indexes;
	AddVertsForInvertedIndexedZSphere3D(verts, indexes, Vec3::ZERO, 10.f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, numSlices, numSlices / 2);
	m_vbo = m_renderer->CreateVertexBuffer((unsigned int)(verts.size() * sizeof(Vertex_PCU)), sizeof(Vertex_PCU));
	m_ibo = m_renderer->CreateIndexBuffer((unsigned int)(indexes.size() * sizeof(unsigned int)));
	m_renderer->CopyCPUToGPU(verts.data(), (unsigned int)(verts.size() * sizeof(Vertex_PCU)), m_vbo);
	m_renderer->CopyCPUToGPU(indexes.data(), (unsigned int)(indexes.size() * sizeof(unsigned int)), m_ibo);
}

#endif // RENDERER_DX12

SkyboxSphere::~SkyboxSphere()
{
}

