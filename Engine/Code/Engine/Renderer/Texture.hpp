#pragma once
#include<string>
#include "Engine/Math/IntVec2.hpp"

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D12Resource;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

class Texture
{
	friend class Renderer; // Only the Renderer can create new Texture objects!
	friend class RendererDX11;
	friend class RendererDX12;

private:
	Texture() {} // can't instantiate directly; must ask Renderer to do it for you
	Texture(Texture const& copy) = delete; // No copying allowed!  This represents GPU memory.
	~Texture() {};

public:
	IntVec2				GetDimensions() const { return m_dimensions; }
	std::string const& GetImageFilePath() const { return m_name; }

	bool IsRenderTarget() const { return m_renderTargetView != nullptr; }
	bool IsDepthTexture() const { return m_depthStencilView != nullptr; }
	bool HasMipLevels() const {return m_numMipLevels != 1;}

protected:
	std::string			m_name;
	IntVec2				m_dimensions;
	unsigned int		m_numMipLevels = 1;

	ID3D11Texture2D* m_texture = nullptr;
	ID3D11ShaderResourceView* m_shaderResourceView = nullptr;
	ID3D11RenderTargetView* m_renderTargetView = nullptr;
	ID3D11DepthStencilView* m_depthStencilView = nullptr;


};

