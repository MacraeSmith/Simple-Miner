#include "Engine/Renderer/VertexBufferDX12.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/CommandQueue.hpp"

VertexBufferDX12::VertexBufferDX12(RendererDX12 const* renderer, std::string const& name)
	:BufferDX12(renderer, name)
	,m_numVertices(0)
	,m_stride(0)
{
}

VertexBufferDX12::~VertexBufferDX12()
{
}

unsigned int VertexBufferDX12::GetStride() const
{
	return m_stride;
}


unsigned int VertexBufferDX12::GetNumberVerts() const
{
	return m_numVertices;
}

void VertexBufferDX12::CreateViews(size_t numElements, size_t elementSize)
{
	m_numVertices = (unsigned int)numElements;
	m_stride = (unsigned int) elementSize;

	m_vertexBufferView.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = static_cast<unsigned int>(m_numVertices * m_stride);
	m_vertexBufferView.StrideInBytes = static_cast<unsigned int>(m_stride);
}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBufferDX12::GetShaderResourceView(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const
{
	UNUSED(srvDesc);
	ERROR_AND_DIE("VertexBufferDX12::GetShaderResourceView() should not be called");

}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBufferDX12::GetUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc) const
{
	UNUSED(uavDesc);
	ERROR_AND_DIE("VertexBufferDX12::GetUnorderedAccessView() should not be called");
}

