#pragma once
#include "Engine/Renderer/BufferDX12.hpp"
#include <d3d12.h>
class IndexBufferDX12 : public BufferDX12
{
	friend class RendererDX12;
public:

	IndexBufferDX12(RendererDX12 const* renderer, std::string const& name = "Added Index Buffer");
	virtual ~IndexBufferDX12();

	virtual void CreateViews(size_t numElements, size_t elementSize) override;

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc = nullptr) const override;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc = nullptr) const override;

	unsigned int			GetNumIndexes() const {return m_numIndexes;}
	unsigned int			GetStride() const {return m_stride;}
	size_t					GetSizeInBytes() const {return m_stride * m_numIndexes;}
	DXGI_FORMAT				GetIndexFormat() const{return m_indexFormat;}
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const{return m_indexBufferView;}

public:

	DXGI_FORMAT				m_indexFormat;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};
	unsigned int			m_stride = 0;
	unsigned int			m_numIndexes = 0;
};

