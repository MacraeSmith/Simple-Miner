#include "Engine/Renderer/ShaderDX12.hpp"
#include "Engine/Renderer/RendererDX12.hpp"

ShaderDX12::ShaderDX12(ShaderConfig const& config)
	:m_config(config)
{
}

ShaderDX12::~ShaderDX12()
{
	DX_SAFE_RELEASE(m_vertexShaderBlob);
	DX_SAFE_RELEASE(m_pixelShaderBlob);
}

std::string const& ShaderDX12::GetName() const
{
	return m_config.m_name;
}

bool ShaderDX12::IsComputeShader() const
{
	return m_computeShaderBlob != nullptr;
}
