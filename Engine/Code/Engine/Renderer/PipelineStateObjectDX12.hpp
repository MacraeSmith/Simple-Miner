#pragma once
#include <string>

struct ID3D12PipelineState;
class ShaderDX12;
class RootSignatureDX12;

class PipelineStateObjectDX12
{
public:
	explicit PipelineStateObjectDX12(ShaderDX12 const* shader, RootSignatureDX12 const* rootSignature, std::string const& name);
	virtual ~PipelineStateObjectDX12();

	std::string GetName() const {return m_name;}

public:
	ID3D12PipelineState* m_pipelineState = nullptr;
	ShaderDX12 const* m_shader = nullptr;
	RootSignatureDX12 const* m_rootSignature = nullptr;
	std::string m_name;

};

