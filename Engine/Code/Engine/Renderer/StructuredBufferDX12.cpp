#include "Engine/Renderer/StructuredBufferDX12.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/DescriptorAllocation.hpp"

StructuredBufferDX12::StructuredBufferDX12(RendererDX12* renderer, D3D12_RESOURCE_DESC const& resourceDesc, std::string const& name)
	:BufferDX12(renderer, resourceDesc, name)
	,m_nonConstRenderer(renderer)
{
}

StructuredBufferDX12::~StructuredBufferDX12()
{
	m_nonConstRenderer->ClearStructuredBufferAtIndex(m_bindlessIndex);
	m_nonConstRenderer = nullptr;
}

void StructuredBufferDX12::CreateViews(size_t numElements, size_t elementSize)
{
	m_numElements = (unsigned int)numElements;
	m_stride = (unsigned int)elementSize;

	ID3D12Device* device = m_renderer->GetDevice();

	//Unordered Access View
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = static_cast<UINT>(numElements);
	uavDesc.Buffer.StructureByteStride = static_cast<UINT>(elementSize);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	m_uavAllocation = m_renderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

	m_uav = m_uavAllocation.GetCPUDescriptorHandle();

	device->CreateUnorderedAccessView(
		m_d3d12Resource,
		nullptr,
		&uavDesc,
		m_uav
	);

	GUARANTEE_OR_DIE((m_uav.ptr != 0), "StructuredBufferDX12 UAV was not created");

	//Shader Resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = static_cast<UINT>(numElements);
	srvDesc.Buffer.StructureByteStride = static_cast<UINT>(elementSize);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	m_srvAllocation = m_renderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
	m_srv = m_srvAllocation.GetCPUDescriptorHandle();
	device->CreateShaderResourceView(m_d3d12Resource, &srvDesc, m_srv);

	GUARANTEE_OR_DIE((m_srv.ptr != 0), "StructuredBufferDX12 SRV was not created");
}

D3D12_CPU_DESCRIPTOR_HANDLE StructuredBufferDX12::GetShaderResourceView(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const
{
	UNUSED(srvDesc);
	return m_srv;
}

D3D12_CPU_DESCRIPTOR_HANDLE StructuredBufferDX12::GetUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc) const
{
	UNUSED(uavDesc);
	return m_uav;
}

StructuredBufferViewDX12::StructuredBufferViewDX12(RendererDX12 const* renderer, ID3D12Resource* resource, unsigned int numElements, unsigned int elementStride)
	: m_renderer(renderer)
	, m_resource(resource)
	, m_numElements(numElements)
	, m_stride(elementStride)
{
}

D3D12_CPU_DESCRIPTOR_HANDLE StructuredBufferViewDX12::GetShaderResourceView() const
{
	if (!m_hasSrv)
	{
		ID3D12Device* device = m_renderer->GetDevice();

		m_srvAllocation = m_renderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
		m_srv = m_srvAllocation.GetCPUDescriptorHandle();
		m_hasSrv = true;

		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format = DXGI_FORMAT_UNKNOWN;

		desc.Buffer.FirstElement = 0;
		desc.Buffer.NumElements = m_numElements;
		desc.Buffer.StructureByteStride = m_stride;
		desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		device->CreateShaderResourceView(m_resource, &desc, m_srv);
	}

	return m_srv;
}
