#include "Engine/Renderer/CommandList.hpp"
#include "Engine/Renderer/CommandQueue.hpp"
#include "Engine/Renderer/DynamicDescriptorHeap.hpp"
#include "Engine/Renderer/ResourceStateTracker.hpp"
#include "Engine/Renderer/UploadBufferDX12.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/ResourceDX12.hpp"
#include "Engine/Renderer/VertexBufferDX12.hpp"
#include "Engine/Renderer/IndexBufferDX12.hpp"
#include "Engine/Renderer/StructuredBufferDX12.hpp"
#include "Engine/Renderer/RootSignatureDX12.hpp"
#include "Engine/Renderer/PipelineStateObjectDX12.hpp"
#include "Engine/Renderer/TextureDX12.hpp"
#include "Engine/Renderer/ResourceDX12.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"
#include <cassert>

std::map<std::string, ID3D12Resource* > CommandList::ms_textureCache;
std::mutex CommandList::ms_textureCacheMutex;

CommandList::CommandList(RendererDX12* renderer, CommandQueue* parentCommandQueue, D3D12_COMMAND_LIST_TYPE type, std::string const& name)
	:m_d3d12CommandListType(type)
	,m_renderer(renderer)
	,m_name(name)
	,m_owningCommandQueue(parentCommandQueue)
{
	ID3D12Device* device = m_renderer->GetDevice();

	HRESULT hr = device->CreateCommandAllocator(m_d3d12CommandListType, IID_PPV_ARGS(&m_d3d12CommandAllocator));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not create command allocator");
	SetCommandAllocatorName(m_d3d12CommandAllocator, Stringf("%s - Command Allocator", name.c_str()));

	hr = device->CreateCommandList(0, m_d3d12CommandListType, m_d3d12CommandAllocator, nullptr, IID_PPV_ARGS(&m_d3d12CommandList));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not create command list");
	SetCommandListName(m_d3d12CommandList, name);

	m_uploadBuffer = std::make_unique<UploadBufferDX12>(m_renderer);

	m_resourceStateTracker = std::make_unique<ResourceStateTracker>();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i] = std::make_unique<DynamicDescriptorHeap>(m_renderer, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
		m_descriptorHeaps[i] = nullptr;
	}
}

CommandList::~CommandList()
{

	DX_SAFE_RELEASE(m_d3d12CommandAllocator);
	DX_SAFE_RELEASE(m_d3d12CommandList);
	DX_SAFE_RELEASE(m_rootSignature);

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		DX_SAFE_RELEASE(m_descriptorHeaps[i]);
	}

	for (int i = 0; i < (int)m_trackedObjects.size(); ++i)
	{
		DX_SAFE_RELEASE(m_trackedObjects[i]);
	}

	m_resourceStateTracker->Reset();
	m_uploadBuffer->Reset();
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i].reset();
	}
}

DynamicDescriptorHeap* CommandList::GetDynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	return m_dynamicDescriptorHeap[type].get();
}

void CommandList::SetName(std::string const& tempName)
{
	m_name = tempName;
	SetCommandListName(m_d3d12CommandList, tempName);
}

void CommandList::TransitionBarrier(const ResourceDX12& resource, D3D12_RESOURCE_STATES stateAfter, unsigned int subResource, bool flushBarriers)
{
	ID3D12Resource* d3d12Resource = resource.GetD3D12Resource();
	if (d3d12Resource)
	{
		// The "before" state is not important. It will be resolved by the resource state tracker.
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = d3d12Resource;
		barrier.Transition.Subresource = subResource;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		barrier.Transition.StateAfter = stateAfter;
		m_resourceStateTracker->ResourceBarrier(barrier);
		
	}

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, unsigned int subResource, bool flushBarriers)
{

	// The "before" state is not important. It will be resolved by the resource state tracker.
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.Subresource = subResource;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter = stateAfter;
	m_resourceStateTracker->ResourceBarrier(barrier);

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::UAVBarrier(ResourceDX12 const& resource, bool flushBarriers)
{
	ID3D12Resource* d3d12Resource = resource.GetD3D12Resource();
	if (d3d12Resource)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.UAV.pResource = d3d12Resource;

		m_resourceStateTracker->ResourceBarrier(barrier);
	}

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::StageShaderResourceViewHandleOnly(uint32_t rootParameterIndex, uint32_t descriptorOffset, D3D12_CPU_DESCRIPTOR_HANDLE shaderResourceViewHandle)
{
	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, shaderResourceViewHandle);
}

void CommandList::CopyResource(ResourceDX12& dstRes, const ResourceDX12& srcRes)
{
	TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

	FlushResourceBarriers();

	m_d3d12CommandList->CopyResource(dstRes.GetD3D12Resource(), srcRes.GetD3D12Resource());

	TrackResource(dstRes);
	TrackResource(srcRes);
}

void CommandList::CopyBuffer(BufferDX12& buffer, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	ID3D12Device* device = m_renderer->GetDevice();
	size_t bufferSize = numElements * elementSize;
	ID3D12Resource* d3d12Resource = nullptr;

	if (bufferSize != 0) // if buffer size == 0 a "null resource" will be created
	{

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = bufferSize;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = flags;

		HRESULT hr =  device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&d3d12Resource));
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not create committed resource in CommandList::CopyBuffer()");

		ResourceStateTracker::AddGlobalResourceState(d3d12Resource, D3D12_RESOURCE_STATE_COMMON);

		if (bufferData != nullptr)
		{
			ID3D12Resource* uploadResource = nullptr;

			D3D12_HEAP_PROPERTIES heapPropsUploadResource = {};
			heapPropsUploadResource.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapPropsUploadResource.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapPropsUploadResource.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapPropsUploadResource.CreationNodeMask = 1;
			heapPropsUploadResource.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC resourceDescUploadResource = {};
			resourceDescUploadResource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDescUploadResource.Alignment = 0;
			resourceDescUploadResource.Width = bufferSize;
			resourceDescUploadResource.Height = 1;
			resourceDescUploadResource.DepthOrArraySize = 1;
			resourceDescUploadResource.MipLevels = 1;
			resourceDescUploadResource.Format = DXGI_FORMAT_UNKNOWN;
			resourceDescUploadResource.SampleDesc.Count = 1;
			resourceDescUploadResource.SampleDesc.Quality = 0;
			resourceDescUploadResource.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDescUploadResource.Flags = D3D12_RESOURCE_FLAG_NONE;

			hr = device->CreateCommittedResource(&heapPropsUploadResource, D3D12_HEAP_FLAG_NONE, &resourceDescUploadResource, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadResource));
			GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not create committed resource for upload resouce in CommandList::CopyBuffer()");

			D3D12_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pData = bufferData;
			subresourceData.RowPitch = bufferSize;
			subresourceData.SlicePitch = subresourceData.RowPitch;

			m_resourceStateTracker->TransitionResource(d3d12Resource, D3D12_RESOURCE_STATE_COPY_DEST);
			FlushResourceBarriers();

			UpdateSubresources(m_d3d12CommandList, d3d12Resource, uploadResource, 0, 0, 1, &subresourceData);

			SetResourceName(uploadResource, "Upload_Resource");
			TrackObject(uploadResource);
			TrackIntermediateObject(uploadResource);
		}

		TrackObject(d3d12Resource);
	}

	buffer.SetD3D12Resource(d3d12Resource);
	buffer.CreateViews(numElements, elementSize);
}

void CommandList::CopyVertexBuffer(VertexBufferDX12& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData)
{
	CopyBuffer(vertexBuffer, numVertices, vertexStride, vertexBufferData);
}

void CommandList::CopyIndexBuffer(IndexBufferDX12& indexBuffer, size_t numIndexes, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
	size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
	CopyBuffer(indexBuffer, numIndexes, indexSizeInBytes, indexBufferData);
}

void CommandList::CopyStructuredBuffer(StructuredBufferDX12& structuredBuffer, size_t numElements, size_t elementSize, const void* structuredBufferData, bool uavWriteAcess)
{
	D3D12_RESOURCE_FLAGS flags = uavWriteAcess
		? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		: D3D12_RESOURCE_FLAG_NONE;

	CopyBuffer(structuredBuffer, numElements, elementSize, structuredBufferData, flags);
}

void CommandList::UploadIntoExistingStructuredBuffer(StructuredBufferDX12& dst, size_t numElements, size_t elementSize, const void* srcData)
{
	ID3D12Device* device = m_renderer->GetDevice();

	ID3D12Resource* dstRes = dst.GetD3D12Resource();
	GUARANTEE_OR_DIE((dstRes != nullptr), "dst structured buffer has no resource");
	GUARANTEE_OR_DIE((srcData != nullptr), "srcData is null");
	GUARANTEE_OR_DIE((numElements <= dst.GetNumElements()), "structured buffer upload exceeds destination capacity");
	GUARANTEE_OR_DIE((elementSize == dst.GetStride()), "structured buffer upload stride mismatch");

	const UINT64 bufferSize = (UINT64)(numElements * elementSize);
	GUARANTEE_OR_DIE((bufferSize > 0), "bufferSize is 0");

	// Create upload buffer
	ID3D12Resource* uploadRes = nullptr;

	D3D12_HEAP_PROPERTIES uploadHeap = {};
	uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeap.CreationNodeMask = 1;
	uploadHeap.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC uploadDesc = {};
	uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadDesc.Alignment = 0;
	uploadDesc.Width = bufferSize;
	uploadDesc.Height = 1;
	uploadDesc.DepthOrArraySize = 1;
	uploadDesc.MipLevels = 1;
	uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadDesc.SampleDesc.Count = 1;
	uploadDesc.SampleDesc.Quality = 0;
	uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = device->CreateCommittedResource(
		&uploadHeap,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadRes)
	);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "failed to create upload resource");

	// Fill upload buffer
	void* mapped = nullptr;
	D3D12_RANGE readRange = {};
	hr = uploadRes->Map(0, &readRange, &mapped);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "failed to map upload resource");

	memcpy(mapped, srcData, (size_t)bufferSize);

	D3D12_RANGE writeRange = { 0, (SIZE_T)bufferSize };
	uploadRes->Unmap(0, &writeRange);

	// Copy into DEFAULT buffer
	m_resourceStateTracker->TransitionResource(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
	FlushResourceBarriers();

	m_d3d12CommandList->CopyBufferRegion(dstRes, 0, uploadRes, 0, bufferSize);

	// IMPORTANT: do NOT transition to SRV here if this is on the COPY queue unless your cross-queue state tracking is rock solid.
	// If you want minimal risk, transition on the DIRECT queue right before draw.

	TrackObject(uploadRes);
	TrackIntermediateObject(uploadRes);
}


void CommandList::FlushResourceBarriers()
{
	m_resourceStateTracker->FlushResourceBarriers(*this);
}

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
	// Constant buffers must be 256-byte aligned.
	Allocation heapAllococation = m_uploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

	m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

void CommandList::SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
	m_d3d12CommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void CommandList::SetComputeDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
	// Constant buffers must be 256-byte aligned.
	Allocation heapAllococation = m_uploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

	m_d3d12CommandList->SetComputeRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

void CommandList::SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
	m_d3d12CommandList->SetComputeRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void CommandList::SetShaderResourceView(uint32_t rootParameterIndex, uint32_t descriptorOffset, const ResourceDX12& resource, D3D12_RESOURCE_STATES stateAfter,
	unsigned int firstSubresource, unsigned int numSubresources, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
{
	if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (uint32_t i = 0; i < numSubresources; ++i)
		{
			TransitionBarrier(resource, stateAfter, firstSubresource + i);
		}
	}
	else
	{
		TransitionBarrier(resource, stateAfter);
	}

	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, resource.GetShaderResourceView(srv));

	TrackResource(resource);
}

void CommandList::SetShaderResourceViewHandle(uint32_t rootParameterIndex, uint32_t descriptorOffset, ResourceDX12 const& resource, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, D3D12_RESOURCE_STATES stateAfter, unsigned int firstSubresource, unsigned int numSubresources)
{
	if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (uint32_t i = 0; i < numSubresources; ++i)
		{
			TransitionBarrier(resource, stateAfter, firstSubresource + i);
		}
	}
	else
	{
		TransitionBarrier(resource, stateAfter);
	}

	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, srvHandle);

	TrackResource(resource);
}

void CommandList::SetShaderResourceViewArray(uint32_t rootParameterIndex, uint32_t descriptorOffset, const std::vector<ResourceDX12 const*>& resources, D3D12_RESOURCE_STATES stateAfter, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
{
	for (size_t i = 0; i < resources.size(); ++i)
	{
		TransitionBarrier(*resources[i], stateAfter);
	}

	for (size_t i = 0; i < resources.size(); ++i)
	{
		m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset + static_cast<uint32_t>(i), 1, resources[i]->GetShaderResourceView(srv));

		TrackResource(*resources[i]);
	}
}

void CommandList::SetUnorderedAccessView(uint32_t rootParameterIndex, uint32_t descriptorOffset, ResourceDX12 const& resource, D3D12_RESOURCE_STATES stateAfter, unsigned int firstSubresource, unsigned int numSubresources, D3D12_UNORDERED_ACCESS_VIEW_DESC const* uav)
{
	if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (uint32_t i = 0; i < numSubresources; ++i)
		{
			TransitionBarrier(resource, stateAfter, firstSubresource + i);
		}
	}
	else
	{
		TransitionBarrier(resource, stateAfter);
	}

	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex,descriptorOffset,1,resource.GetUnorderedAccessView(uav));

	TrackResource(resource);
}

void CommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	m_d3d12CommandList->IASetPrimitiveTopology(primitiveTopology);
}

void CommandList::SetVertexBuffer(uint32_t slot, VertexBufferDX12 const& vertexBuffer)
{
	TransitionBarrier(vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = vertexBuffer.GetVertexBufferView();
	m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
	TrackResource(vertexBuffer);
}

void CommandList::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
{
	size_t bufferSize = numVertices * vertexSize;

	Allocation heapAllocation = m_uploadBuffer->Allocate(bufferSize, vertexSize);
	memcpy(heapAllocation.CPU, vertexBufferData, bufferSize);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = heapAllocation.GPU;
	vertexBufferView.SizeInBytes = (unsigned int)bufferSize;
	vertexBufferView.StrideInBytes = (unsigned int)vertexSize;

	m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void CommandList::SetIndexBuffer(IndexBufferDX12 const& indexBuffer)
{
	TransitionBarrier( indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	D3D12_INDEX_BUFFER_VIEW indexBufferView = indexBuffer.GetIndexBufferView();
	m_d3d12CommandList->IASetIndexBuffer(&indexBufferView);
	TrackResource(indexBuffer);
}

void CommandList::SetDynamicIndexBuffer(size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
	size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
	size_t bufferSize = numIndicies * indexSizeInBytes;

	Allocation heapAllocation = m_uploadBuffer->Allocate(bufferSize, indexSizeInBytes);
	memcpy(heapAllocation.CPU, indexBufferData, bufferSize);

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = heapAllocation.GPU;
	indexBufferView.SizeInBytes = static_cast<unsigned int>(bufferSize);
	indexBufferView.Format = indexFormat;

	m_d3d12CommandList->IASetIndexBuffer(&indexBufferView);
}

void CommandList::SetViewport(const D3D12_VIEWPORT& viewport)
{
	SetViewports({ viewport });
}

void CommandList::SetViewports(const std::vector<D3D12_VIEWPORT>& viewports)
{
	assert(viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
	m_d3d12CommandList->RSSetViewports(static_cast<unsigned int>(viewports.size()), viewports.data());
}

void CommandList::SetScissorRect(const D3D12_RECT& scissorRect)
{
	SetScissorRects({ scissorRect });
}

void CommandList::SetScissorRects(const std::vector<D3D12_RECT>& scissorRects)
{
	assert(scissorRects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
	m_d3d12CommandList->RSSetScissorRects(static_cast<unsigned int>(scissorRects.size()), scissorRects.data());
}

void CommandList::SetPipelineState(ID3D12PipelineState* pipelineState)
{
	m_d3d12CommandList->SetPipelineState(pipelineState);
	TrackObject(pipelineState);
}

void CommandList::SetGraphicsRootSignature(const RootSignatureDX12& rootSignature)
{
	ID3D12RootSignature* d3d12RootSignature = rootSignature.GetRootSignature();
	if (m_rootSignature != d3d12RootSignature)
	{
		m_rootSignature = d3d12RootSignature;

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_dynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
		}

		m_d3d12CommandList->SetGraphicsRootSignature(m_rootSignature);

		TrackObject(m_rootSignature);
	}
}

void CommandList::SetComputeRootSignature(const RootSignatureDX12& rootSignature)
{
	ID3D12RootSignature* d3d12RootSignature = rootSignature.GetRootSignature();
	if (m_rootSignature != d3d12RootSignature)
	{
		m_rootSignature = d3d12RootSignature;

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_dynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
		}

		m_d3d12CommandList->SetComputeRootSignature(m_rootSignature);

		TrackObject(m_rootSignature);
	}
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
	FlushResourceBarriers();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	m_d3d12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
	FlushResourceBarriers();
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	m_d3d12CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void CommandList::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
{
	FlushResourceBarriers();

	/*
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
 		m_dynamicDescriptorHeap[i]->CommitStagedDescriptorsForDispatch(*this);
	}

	*/

	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->CommitStagedDescriptorsForDispatch(*this);
	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]->CommitStagedDescriptorsForDispatch(*this);

	m_d3d12CommandList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
}

bool CommandList::Close(CommandList& pendingCommandList)
{
	if (m_owningCommandQueue)
	{
		SetName(Stringf("%s Command List", m_owningCommandQueue->m_name.c_str()));
	}

	// Flush any remaining barriers.
	FlushResourceBarriers();
	// Flush pending resource barriers.
	uint32_t numPendingBarriers = m_resourceStateTracker->FlushPendingResourceBarriers(pendingCommandList);

	m_d3d12CommandList->Close();

	// Commit the final resource state to the global state.
	m_resourceStateTracker->CommitFinalResourceStates();

	return numPendingBarriers > 0;
}

void CommandList::Close()
{
	if (m_owningCommandQueue)
	{
		SetName(Stringf("%s Command List", m_owningCommandQueue->m_name.c_str()));
	}

	m_resourceStateTracker->FlushPendingResourceBarriers(*this);
	FlushResourceBarriers();
	m_d3d12CommandList->Close();
}

void CommandList::Reset()
{
	HRESULT hr = m_d3d12CommandAllocator->Reset();

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could Not Reset command allocator");

	hr = m_d3d12CommandList->Reset(m_d3d12CommandAllocator, nullptr);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not reset command list");

	m_resourceStateTracker->Reset();
	m_uploadBuffer->Reset();

	ReleaseTrackedObjects();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i]->Reset();
		m_descriptorHeaps[i] = nullptr;
	}

	m_rootSignature = nullptr;
	//m_ComputeCommandList = nullptr;
}

void CommandList::ReleaseTrackedObjects()
{
	m_trackedObjects.clear();

	for (int i = 0; i < (int)m_intermediateObjects.size(); ++i)
	{
		if (m_intermediateObjects[i])
		{
			DX_SAFE_RELEASE(m_intermediateObjects[i]);
		}
	}
}

void CommandList::LoadTextureFromFile(TextureDX12& texture, std::string const& filePathWithExtension)
{

	std::lock_guard<std::mutex> lock(ms_textureCacheMutex);
	std::string fileName = GetFileName(filePathWithExtension);
	auto iter = ms_textureCache.find(fileName);
	if (iter != ms_textureCache.end())
	{
		texture.SetD3D12Resource(iter->second);
		texture.CreateViews();
		texture.SetName(fileName);
	}

	else
	{
		Image newTextureImage(filePathWithExtension.c_str());
		IntVec2 dims = newTextureImage.GetDimensions();
		texture.SetDimensions(dims);

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Alignment = 0; // Let D3D12 choose the default alignment
		textureDesc.Width = (unsigned int)dims.x;
		textureDesc.Height = (unsigned int)dims.y;
		textureDesc.DepthOrArraySize = 1;	// default to one texture in array
		textureDesc.MipLevels = 1;			//default to no mips
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //will work with .tga, .jpg, .png but not .dds
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ID3D12Device* device = m_renderer->GetDevice();
		ID3D12Resource* textureResource;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&textureResource));
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture resource");

		texture.SetD3D12Resource(textureResource);
		texture.CreateViews();
		texture.SetName(fileName);

		TextureUploadInfo uploadInfo = CreateTextureUploadBuffer(device, textureResource);
		Rgba8* texelData = (Rgba8*)newTextureImage.GetRawData();

		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData = texelData;
		subResourceData.RowPitch = dims.x * sizeof(Rgba8);
		subResourceData.SlicePitch = subResourceData.RowPitch * dims.y;

		UpdateSubresources(m_d3d12CommandList, textureResource, uploadInfo.m_uploadHeap, 0, 0, 1, &subResourceData);

		TransitionBarrier(texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		ResourceStateTracker::AddGlobalResourceState(textureResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		ms_textureCache[fileName] = textureResource;

		TrackIntermediateObject(uploadInfo.m_uploadHeap);
	}
}

void CommandList::LoadTextureFromFileWithMips(TextureDX12& texture, std::string const& filePathWithExtension, int numMipLayers)
{
	std::lock_guard<std::mutex> lock(ms_textureCacheMutex);

	std::string fileName = GetFileName(filePathWithExtension);
	auto iter = ms_textureCache.find(fileName);
	if (iter != ms_textureCache.end())
	{
		texture.SetD3D12Resource(iter->second);
		texture.CreateViews();
		texture.SetName(fileName);
		return;
	}

	Image newTextureImage(filePathWithExtension.c_str());
	IntVec2 dimensions = newTextureImage.GetDimensions();
	texture.SetDimensions(dimensions);

	uint16_t destinationMipCount = CalculateDestinationMipCount(dimensions, numMipLayers);

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0;
	textureDesc.Width = (unsigned int)(dimensions.x);
	textureDesc.Height = (unsigned int)(dimensions.y);
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = destinationMipCount;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = (destinationMipCount > 1) ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

	ID3D12Device* device = m_renderer->GetDevice();
	ID3D12Resource* textureResource = nullptr;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&textureResource));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture resource");
	ResourceStateTracker::AddGlobalResourceState(textureResource, D3D12_RESOURCE_STATE_COMMON);

	texture.SetD3D12Resource(textureResource);
	texture.SetName(fileName);

	TransitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
	TextureUploadInfo uploadInfo = CreateTextureUploadBuffer(device, textureResource);

	Rgba8 const* texelData = (Rgba8 const*)(newTextureImage.GetRawData());
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = texelData;
	subResourceData.RowPitch = (LONG_PTR)(dimensions.x * (int)(sizeof(Rgba8)));
	subResourceData.SlicePitch = (LONG_PTR)(subResourceData.RowPitch * (LONG_PTR)(dimensions.y));

	UpdateSubresources(m_d3d12CommandList, textureResource, uploadInfo.m_uploadHeap, 0, 0, 1, &subResourceData);
	texture.CreateViews();

	if (destinationMipCount > 1)
	{
		m_renderer->QueueTextureForMipGeneration(&texture, dimensions, destinationMipCount);
	}
	else
	{
		TransitionBarrier(texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
	}

	ResourceStateTracker::AddGlobalResourceState(textureResource, D3D12_RESOURCE_STATE_COPY_DEST);

	ms_textureCache[fileName] = textureResource;
	TrackIntermediateObject(uploadInfo.m_uploadHeap);
}

void CommandList::LoadTextureFromImage(TextureDX12& texture, std::string const& imageName, Image const& image)
{
	std::lock_guard<std::mutex> lock(ms_textureCacheMutex);
	auto iter = ms_textureCache.find(imageName);
	if (iter != ms_textureCache.end())
	{
		texture.SetD3D12Resource(iter->second);
		texture.CreateViews();
		texture.SetName(imageName);
	}

	else
	{
		IntVec2 dims = image.GetDimensions();
		texture.SetDimensions(dims);

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Alignment = 0; // Let D3D12 choose the default alignment
		textureDesc.Width = (unsigned int)dims.x;
		textureDesc.Height = (unsigned int)dims.y;
		textureDesc.DepthOrArraySize = 1;	// default to one texture in array
		textureDesc.MipLevels = 1;			//default to no mips
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //will work with .tga, .jpg, .png but not .dds
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ID3D12Device* device = m_renderer->GetDevice();
		ID3D12Resource* textureResource;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&textureResource));
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture resource");

		texture.SetD3D12Resource(textureResource);
		texture.CreateViews();
		texture.SetName(imageName);

		TextureUploadInfo uploadInfo = CreateTextureUploadBuffer(device, textureResource);
		Rgba8* texelData = (Rgba8*)image.GetRawData();

		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData = texelData;
		subResourceData.RowPitch = dims.x * sizeof(Rgba8);
		subResourceData.SlicePitch = subResourceData.RowPitch * dims.y;

		UpdateSubresources(m_d3d12CommandList, textureResource, uploadInfo.m_uploadHeap, 0, 0, 1, &subResourceData);

		TransitionBarrier(texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		ResourceStateTracker::AddGlobalResourceState(textureResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		ms_textureCache[imageName] = textureResource;

		TrackIntermediateObject(uploadInfo.m_uploadHeap);
	}
}

void CommandList::LoadTextureFromImageWithMips(TextureDX12& texture, std::string const& imageName, Image const& image, int numMipLayers)
{
	std::lock_guard<std::mutex> lock(ms_textureCacheMutex);

	auto iter = ms_textureCache.find(imageName);
	if (iter != ms_textureCache.end())
	{
		texture.SetD3D12Resource(iter->second);
		texture.CreateViews();
		texture.SetName(imageName);
		return;
	}

	IntVec2 dimensions = image.GetDimensions();
	texture.SetDimensions(dimensions);

	uint16_t destinationMipCount = CalculateDestinationMipCount(dimensions, numMipLayers);

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0;
	textureDesc.Width = (unsigned int)(dimensions.x);
	textureDesc.Height = (unsigned int)(dimensions.y);
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = destinationMipCount;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = (destinationMipCount > 1) ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

	ID3D12Device* device = m_renderer->GetDevice();
	ID3D12Resource* textureResource = nullptr;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&textureResource));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture resource");

	// Track the true initial state.
	ResourceStateTracker::AddGlobalResourceState(textureResource, D3D12_RESOURCE_STATE_COMMON);

	texture.SetD3D12Resource(textureResource);
	texture.SetName(imageName);

	TransitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
	TextureUploadInfo uploadInfo = CreateTextureUploadBuffer(device, textureResource);

	Rgba8 const* texelData = (Rgba8 const*)(image.GetRawData());
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = texelData;
	subResourceData.RowPitch = (LONG_PTR)(dimensions.x * (int)(sizeof(Rgba8)));
	subResourceData.SlicePitch = (LONG_PTR)(subResourceData.RowPitch * (LONG_PTR)(dimensions.y));

	UpdateSubresources(m_d3d12CommandList, textureResource, uploadInfo.m_uploadHeap, 0, 0, 1, &subResourceData);
	texture.CreateViews();

	if (destinationMipCount > 1)
	{
		m_renderer->QueueTextureForMipGeneration(&texture, dimensions, destinationMipCount);
	}
	else
	{
		TransitionBarrier(texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
	}

	ResourceStateTracker::AddGlobalResourceState(textureResource, D3D12_RESOURCE_STATE_COPY_DEST);

	ms_textureCache[imageName] = textureResource;
	TrackIntermediateObject(uploadInfo.m_uploadHeap);
}

void CommandList::BeginRendererEvent(char const* eventName) const
{
	int eventNameLength = (int)strlen(eventName) + 1;
	int eventNameWideCharLength = MultiByteToWideChar(CP_UTF8, 0, eventName, eventNameLength, nullptr, 0);

	std::wstring eventNameWide(eventNameWideCharLength, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, eventName, eventNameLength, &eventNameWide[0], eventNameWideCharLength);
	m_d3d12CommandList->BeginEvent(0, eventNameWide.c_str(), (unsigned int)eventNameWide.size() * sizeof(wchar_t));
}

void CommandList::EndRendererEvent() const
{
	m_d3d12CommandList->EndEvent();
}


void CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap)
{
	if (m_descriptorHeaps[heapType] != heap)
	{
		m_descriptorHeaps[heapType] = heap;
		BindDescriptorHeaps();
	}
}

void CommandList::TrackObject(ID3D12Object* object)
{
	m_trackedObjects.push_back(object);
}

void CommandList::TrackResource(const ResourceDX12& res)
{
	TrackObject(res.GetD3D12Resource());
}

void CommandList::TrackIntermediateResource(ResourceDX12 const& res)
{
	TrackIntermediateObject(res.GetD3D12Resource());
}

void CommandList::TrackIntermediateObject(ID3D12Object* object)
{
	m_intermediateObjects.push_back(object);
}

void CommandList::BindDescriptorHeaps()
{
	unsigned int numDescriptorHeaps = 0;
	ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
	std::string heapNames[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {"CBV_SRV_UAV", "SAMPLER", "RTV", "DSV"};

	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		ID3D12DescriptorHeap* descriptorHeap = m_descriptorHeaps[i];
		SetDescriptorHeapName(descriptorHeap, Stringf("%s: CommandList::BindDescriptorHeaps() DescriptorHeap:  %s", m_name.c_str(), heapNames[i].c_str()));
		if (descriptorHeap)
		{
			descriptorHeaps[numDescriptorHeaps++] = descriptorHeap;
		}
	}

	m_d3d12CommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
}

TextureUploadInfo CommandList::CreateTextureUploadBuffer(ID3D12Device* device, ID3D12Resource* textureResource)
{
	TextureUploadInfo uploadInfo = {};
	D3D12_RESOURCE_DESC textureDesc = textureResource->GetDesc();

	device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &uploadInfo.m_footprint, &uploadInfo.m_numRows, &uploadInfo.m_rowSizeInBytes, &uploadInfo.m_totalBytes);

	D3D12_RESOURCE_DESC uploadDesc = {};
	uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadDesc.Alignment = 0;
	uploadDesc.Width = uploadInfo.m_totalBytes;
	uploadDesc.Height = 1;
	uploadDesc.DepthOrArraySize = 1;
	uploadDesc.MipLevels = 1;
	uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadDesc.SampleDesc.Count = 1;
	uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES uploadHeapProps = {};
	uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProps.CreationNodeMask = 1;
	uploadHeapProps.VisibleNodeMask = 1;


	ID3D12Resource* uploadResource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadResource));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create upload resource");

	uploadInfo.m_uploadHeap = uploadResource;
	return uploadInfo;
}

uint16_t CommandList::CalculateDestinationMipCount(IntVec2 const& dimensions, int requestedMipLevels)
{
	int fullMipCount = CalculateFullMipCount2D(dimensions.x, dimensions.y);

	if (requestedMipLevels == 0)
	{
		return (uint16_t)(fullMipCount);
	}

	int clampedMipLevels = requestedMipLevels;
	if (clampedMipLevels < 1)
	{
		clampedMipLevels = 1;
	}

	if (clampedMipLevels > fullMipCount)
	{
		clampedMipLevels = fullMipCount;
	}

	return (uint16_t)(clampedMipLevels);
}
