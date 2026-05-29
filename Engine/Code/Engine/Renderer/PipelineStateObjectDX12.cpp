#include "Engine/Renderer/PipelineStateObjectDX12.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Renderer/ShaderDX12.hpp"
#include "Engine/Renderer/RootSignatureDX12.hpp"

PipelineStateObjectDX12::PipelineStateObjectDX12(ShaderDX12 const* shader, RootSignatureDX12 const* rootSignature, std::string const& name)
	:m_shader(shader)
	,m_rootSignature(rootSignature)
	,m_name(name)
{
}

PipelineStateObjectDX12::~PipelineStateObjectDX12()
{
	m_shader = nullptr;
	m_rootSignature = nullptr;
	DX_SAFE_RELEASE(m_pipelineState);
}
