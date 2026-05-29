#pragma once
#include <vector>
#include <string>


#include "Game/EngineBuildPreferences.hpp"
#ifdef RENDERER_DX12
class RendererDX12;
class ShaderDX12;
class TextureDX12;
#else
class RendererDX11;
class Texture;
class Shader;
#endif // RENDERER_DX12

enum class BlendMode : int;
enum class DepthMode : int;
enum class RasterizerMode : int;
class Renderer;

class Material
{
public:
#ifdef RENDERER_DX12
	Material(Renderer* renderer, std::string const& name, ShaderDX12 const* shader, std::vector<TextureDX12 const*> textures = {});
	void SetTextures(std::vector<TextureDX12 const*> textures);
	void SetTextures(TextureDX12 const* texture);
	void SetShader(ShaderDX12 const* shader);
#else
	Material(Renderer* renderer, std::string const& name, Shader const* shader ,std::vector<Texture const*> textures = {});
	void SetTextures(std::vector<Texture const*> textures);
	void SetTextures(Texture const* texture);
	void SetShader(Shader const* shader);
#endif

	Material(Material const* matToCopy, std::string const& name = "CopyName");
	void BindMaterial() const;
	void SetBlendMode(BlendMode blendMode);
	void ClearTextures();

private:

public:
	std::string m_name;
private:

#ifdef RENDERER_DX12
	ShaderDX12 const* m_shader = nullptr;
	std::vector<TextureDX12 const*> m_textures;
	RendererDX12* m_renderer = nullptr;

#else
	Shader const* m_shader = nullptr;
	std::vector<Texture const*> m_textures;
	RendererDX11* m_renderer = nullptr;
#endif

	int m_blendMode = 0;
	int m_depthMode = 3;
	int m_rasterizerMode = 1;
};

