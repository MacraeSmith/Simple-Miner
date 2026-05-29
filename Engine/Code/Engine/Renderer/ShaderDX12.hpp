#pragma once
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include <string>
#include <d3d12.h>


class ShaderDX12
{
	friend class Renderer;
	friend class RendererDX12;
public:
	ShaderDX12(ShaderConfig const& config);
	ShaderDX12(ShaderDX12 const& copy) = delete;
	virtual ~ShaderDX12();

	std::string const& GetName() const;
	bool IsComputeShader() const;

	ID3DBlob* GetVertexBlob() const {return m_vertexShaderBlob;}
	ID3DBlob* GetPixelBlob() const {return m_pixelShaderBlob;}
	ID3DBlob* GetComputeBlob() const {return m_computeShaderBlob;}

private:
	ShaderConfig m_config;
	ID3DBlob* m_vertexShaderBlob = nullptr;
	ID3DBlob* m_pixelShaderBlob = nullptr;
	ID3DBlob* m_computeShaderBlob = nullptr;
	VertexType m_vertexType = VertexType::VERTEX_PCU;


};

