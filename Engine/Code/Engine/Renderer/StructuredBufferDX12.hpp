#pragma once
#include "Engine/Renderer/BufferDX12.hpp"
#include "Engine/Renderer/DescriptorAllocation.hpp"
#include <d3d12.h>
class StructuredBufferDX12 : public BufferDX12
{
friend class RendererDX12;
public:
	StructuredBufferDX12(RendererDX12* renderer, D3D12_RESOURCE_DESC const& resourceDesc, std::string const& name = "AddedBuffer");
	virtual ~StructuredBufferDX12();

	virtual void CreateViews(size_t numElements, size_t elementSize) override;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc = nullptr) const override;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc = nullptr) const override;
	uint32_t GetBindlessIndex() const { return m_bindlessIndex; }
	unsigned int GetNumElements() const {return m_numElements;}
	unsigned int GetStride() const {return m_stride;}


private:

	D3D12_CPU_DESCRIPTOR_HANDLE m_uav = {};
	DescriptorAllocation m_uavAllocation;

	D3D12_CPU_DESCRIPTOR_HANDLE m_srv = {};
	DescriptorAllocation        m_srvAllocation;

	unsigned int m_numElements = 0;
	unsigned int m_stride = 0;
	uint32_t m_bindlessIndex = UINT32_MAX;
	RendererDX12* m_nonConstRenderer = nullptr;

};

class StructuredBufferViewDX12
{
public:
	StructuredBufferViewDX12(RendererDX12 const* renderer,
		ID3D12Resource* resource,
		unsigned int numElements,
		unsigned int elementStride);

	D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView() const;

private:
	RendererDX12 const* m_renderer = nullptr;
	ID3D12Resource* m_resource = nullptr;
	unsigned int        m_numElements = 0;
	unsigned int        m_stride = 0;

	mutable bool                 m_hasSrv = false;
	mutable DescriptorAllocation m_srvAllocation;
	mutable D3D12_CPU_DESCRIPTOR_HANDLE m_srv = {};
};

