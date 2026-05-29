#pragma once
#include "Game/EngineBuildPreferences.hpp"
#include "Engine/Math/Vec3.hpp"
#include <vector>

class Texture;
class TextureDX12;
class Renderer;
class RendererDX11;
class RendererDX12;
class VertexBuffer;
class VertexBufferDX12;
class IndexBuffer;
class IndexBufferDX12;
class Shader;
class ShaderDX12;
class Skybox
{
public:
#ifdef RENDERER_DX12
	explicit Skybox(RendererDX12* renderer);
	explicit Skybox(RendererDX12* renderer, std::vector<TextureDX12 const*> textures);
#else
	explicit Skybox(RendererDX11* renderer);
	explicit Skybox(RendererDX11* renderer, std::vector<Texture const*> textures);
#endif
	virtual ~Skybox();

	void UpdatePosition(Vec3 const& position);
	void Render() const;
public:
#ifdef RENDERER_DX12
	RendererDX12* m_renderer = nullptr;
	std::vector<TextureDX12 const*> m_textures = {};
	VertexBufferDX12* m_vbo = nullptr;
	IndexBufferDX12* m_ibo = nullptr;
	ShaderDX12* m_shader = nullptr;

#else
	RendererDX11* m_renderer = nullptr;
	std::vector<Texture const*> m_textures = {};
	VertexBuffer* m_vbo = nullptr;
	IndexBuffer* m_ibo = nullptr;
	Shader* m_shader = nullptr;
#endif // RENDERER_DX12

	Vec3 m_position = Vec3::ZERO;
	float m_universalScale = 1.f;

};

class SkyboxSphere : public Skybox
{
public:
#ifdef RENDERER_DX12
	explicit SkyboxSphere(RendererDX12* renderer, TextureDX12 const* texture, int numSlices = 32);
#else
	explicit SkyboxSphere(RendererDX11* renderer, Texture const* texture, int numSlices = 32);
#endif
	virtual ~SkyboxSphere();
};

