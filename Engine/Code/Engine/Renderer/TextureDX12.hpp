#pragma once
#include "Engine/Renderer/ResourceDX12.hpp"
#include "Engine/Renderer/DescriptorAllocation.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <unordered_map>

class DynamicDescriptorHeap;

class TextureDX12 : public ResourceDX12
{
	friend class CommandList;
	friend class RendererDX12;
public:
	void Resize(IntVec2 const& dimensions, unsigned int depthOrArraySize = 1);

	D3D12_CPU_DESCRIPTOR_HANDLE			GetRenderTargetView() const;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;
	D3D12_CPU_DESCRIPTOR_HANDLE			GetDepthStencilView() const;

	bool CheckSRVSupport() const {return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE); }
	bool CheckRTVSupport() const {return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_RENDER_TARGET); }
	bool CheckUAVSupport() const 
		{return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) 
		&& CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) 
		&& CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);}

	bool	CheckDSVSupport() const{return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);}
	bool	HasAlpha() const;

	static bool			IsUAVCompatibleFormat(DXGI_FORMAT format);
	static bool			IsSRGBFormat(DXGI_FORMAT format);
	static bool			IsBGRFormat(DXGI_FORMAT format);
	static bool			IsDepthFormat(DXGI_FORMAT format);

	static DXGI_FORMAT	GetTypelessFormat(DXGI_FORMAT format);
	static DXGI_FORMAT	GetSRGBFormat(DXGI_FORMAT format);
	static DXGI_FORMAT	GetUAVCompatableFormat(DXGI_FORMAT format);

	void				SetDimensions(IntVec2 const& dimensions);
	IntVec2				GetDimensions() const {return m_dimensions;}

	void				SetDepth(uint32_t depth){m_depth = depth;}
	uint32_t			GetDepth() const {return m_depth;}

	uint16_t GetMipLevelCount() const;



	uint32_t GetBindlessIndex() const { return m_bindlessIndex; }
protected:
	TextureDX12(RendererDX12* renderer, std::string const& name, D3D12_RESOURCE_DESC const& resourceDesc, D3D12_CLEAR_VALUE const* clearValue = nullptr);
	TextureDX12(RendererDX12* renderer, std::string const& name, ID3D12Resource* resource);
	TextureDX12(RendererDX12* renderer, std::string const& name);
	virtual ~TextureDX12();

	void CreateViews();


	//For Bindless
	D3D12_CPU_DESCRIPTOR_HANDLE GetBindlessCPUHandle() const { return m_cpuDescriptorHandle; }
	void SetBindlessIndex(uint32_t index) { m_bindlessIndex = index; }

	void CreateBindlessSRV(ID3D12DescriptorHeap* bindlessHeap, uint32_t heapIndex, D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr);

private:
	DescriptorAllocation CreateShaderResourceView( D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const;
	DescriptorAllocation CreateUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc) const;

private:
	DescriptorAllocation m_RenderTargetView;
	DescriptorAllocation m_depthStencilView;

	RendererDX12* m_nonConstRenderer = nullptr;

	mutable std::unordered_map<size_t, DescriptorAllocation> m_ShaderResourceViews;
	mutable std::unordered_map<size_t, DescriptorAllocation> m_UnorderedAccessViews;

	mutable std::mutex m_ShaderResourceViewsMutex;
	mutable std::mutex m_UnorderedAccessViewsMutex;

	IntVec2				m_dimensions;
	uint32_t			m_depth = 1; //depth is 1 unless the texture is 3D

	//For Bindless
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuDescriptorHandle{};
	uint32_t m_bindlessIndex = UINT32_MAX;

	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuDescriptorHandle{};
};


