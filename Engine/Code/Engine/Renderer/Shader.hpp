#pragma once
#include <string>

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;

struct ShaderConfig
{
	std::string m_name;
	std::string m_vertexEntryPoint = "VertexMain";
	std::string m_pixelEntryPoint = "PixelMain";
	bool m_computeShader = false;
};

class Shader
{
	friend class Renderer;
	friend class RendererDX11;
public:
	Shader(const ShaderConfig& config);
	Shader(const Shader& copy) = delete;
	~Shader();

	const std::string& GetName() const;

private:
	ShaderConfig m_config;
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
};

