#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dCompiler.lib")
#if defined(ENGINE_DEBUG_RENDERER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

Shader::Shader(const ShaderConfig& config)
	:m_config(config)
{
}

Shader::~Shader()
{
	DX_SAFE_RELEASE(m_vertexShader);
	DX_SAFE_RELEASE(m_pixelShader);
	DX_SAFE_RELEASE(m_inputLayout);
}

const std::string& Shader::GetName() const
{
	return m_config.m_name;
}
