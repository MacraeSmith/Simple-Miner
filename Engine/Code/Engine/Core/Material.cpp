#include "Engine/Core/Material.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/EngineBuildPreferences.hpp"

#ifdef RENDERER_DX12
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Renderer/ShaderDX12.hpp"
#include "Engine/Renderer/TextureDX12.hpp"
#else
#include "Engine/Renderer/RendererDX11.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Texture.hpp"
#endif


#ifdef RENDERER_DX12
Material::Material(Renderer* renderer, std::string const& name, ShaderDX12 const* shader, std::vector<TextureDX12 const*> textures)
	:m_textures(textures)
	,m_name(name)
	,m_shader(shader)
{
	RendererDX12* rendererDX12 = dynamic_cast<RendererDX12*>(renderer);
	if (rendererDX12)
	{
		m_renderer = rendererDX12;
	}

	else
	{
		ERROR_AND_DIE("Trying to create DX12 Static Mesh with DX11 Renderer");
	}
}
void Material::SetTextures(std::vector<TextureDX12 const*> textures)
{
	m_textures = textures;
}
void Material::SetTextures(TextureDX12 const* texture)
{
	m_textures.push_back(texture);
}
void Material::SetShader(ShaderDX12 const* shader)
{
	m_shader = shader;
}
#else
Material::Material(Renderer* renderer, std::string const& name, Shader const* shader, std::vector<Texture const*> textures)
	:m_textures(textures)
	, m_name(name)
	, m_shader(shader)
{
	RendererDX11* rendererDX11 = dynamic_cast<RendererDX11*>(renderer);
	if (rendererDX11)
	{
		m_renderer = rendererDX11;
	}

	else
	{
		ERROR_AND_DIE("Trying to create DX11 Static Mesh with DX12 Renderer");
	}
}



void Material::SetTextures(std::vector<Texture const*> textures)
{
	m_textures = textures;
}
void Material::SetTextures(Texture const* texture)
{
	m_textures.push_back(texture);
}
void Material::SetShader(Shader const* shader)
{
	m_shader = shader;
}
#endif

Material::Material(Material const* matToCopy, std::string const& name)
	:m_shader(matToCopy->m_shader)
	,m_textures(matToCopy->m_textures)
	,m_blendMode(matToCopy->m_blendMode)
	,m_depthMode(matToCopy->m_depthMode)
	,m_rasterizerMode(matToCopy->m_rasterizerMode)
	,m_renderer(matToCopy->m_renderer)
{
	m_name = name == "CopyName" ? Stringf("%s_Copy", matToCopy->m_name.c_str()).c_str() : name;
}

void Material::BindMaterial() const
{
#ifdef RENDERER_DX12
	m_renderer->BindRootSignature(nullptr);
#endif
	m_renderer->SetBlendMode((BlendMode)m_blendMode);
	m_renderer->SetDepthMode((DepthMode)m_depthMode);
	m_renderer->SetRasterizerMode((RasterizerMode)m_rasterizerMode);

	m_renderer->BindShader(m_shader);

	if (m_textures.size() <= 0)
	{
		m_renderer->BindTexture(nullptr);
	}

	else
	{
		int numTextures = GetMin((int)m_textures.size(), NUM_TEXTURE_SLOTS);


		for (int i = 0; i < numTextures; ++i)
		{
			m_renderer->BindTexture(m_textures[i], i);
		}
	}
}

void Material::SetBlendMode(BlendMode blendMode)
{
	m_blendMode = (int)blendMode;
}

void Material::ClearTextures()
{
	m_textures.clear();
}
