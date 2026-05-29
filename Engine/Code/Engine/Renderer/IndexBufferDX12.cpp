#include "Engine/Renderer/IndexBufferDX12.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <cassert>

IndexBufferDX12::IndexBufferDX12(RendererDX12 const* renderer, std::string const& name)
	:BufferDX12(renderer, name)
	,m_numIndexes(0)
	,m_stride(0)
	,m_indexFormat(DXGI_FORMAT_UNKNOWN)
	,m_indexBufferView({})
{
}

IndexBufferDX12::~IndexBufferDX12()
{
}

void IndexBufferDX12::CreateViews(size_t numElements, size_t elementSize)
{
	assert(elementSize == 2 || elementSize == 4 && "Indices must be 16, or 32-bit integers.");

	m_numIndexes = (unsigned int)numElements;
	m_indexFormat = (elementSize == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

	m_indexBufferView.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = static_cast<unsigned int>(numElements * elementSize);
	m_indexBufferView.Format = m_indexFormat;
}

D3D12_CPU_DESCRIPTOR_HANDLE IndexBufferDX12::GetShaderResourceView(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const
{
	UNUSED(srvDesc);
	ERROR_AND_DIE("IndexBufferDX12::GetShaderResourceView() should not be called");
}

D3D12_CPU_DESCRIPTOR_HANDLE IndexBufferDX12::GetUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc) const
{
	UNUSED(uavDesc);
	ERROR_AND_DIE("IndexBufferDX12::GetUnorderedAccessView() should not be called");
}

