#include "Engine/Renderer/ResourceDX12.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/ResourceStateTracker.hpp"
#include "Engine/Renderer/CommandQueue.hpp"

ResourceDX12::ResourceDX12(RendererDX12 const* renderer, std::string const& name)
	:m_renderer(renderer)
	,m_resourceName(name)
	,m_FormatSupport({})
{
}

ResourceDX12::ResourceDX12(RendererDX12 const* renderer, D3D12_RESOURCE_DESC const& resourceDesc, const D3D12_CLEAR_VALUE* clearValue, std::string const& name)
	:m_renderer(renderer)
	,m_resourceName(name)
	, m_FormatSupport({})
{
	ID3D12Device* device = m_renderer->GetDevice();

	if (clearValue)
	{
		m_d3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
	}

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	HRESULT hr = device->CreateCommittedResource(&heapProperties,D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, m_d3d12ClearValue.get(), IID_PPV_ARGS(&m_d3d12Resource));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not create commited resource");

	ResourceStateTracker::AddGlobalResourceState(m_d3d12Resource, D3D12_RESOURCE_STATE_COMMON);

	SetName(name);
}

ResourceDX12::ResourceDX12(RendererDX12 const* renderer,ID3D12Resource* resource, std::string const& name)
	: m_renderer(renderer)
	, m_d3d12Resource(resource)
{
	SetName(name);
}


ResourceDX12::~ResourceDX12()
{
	if (m_d3d12Resource == nullptr)
	{
		return;
	}

	//d3d12 resource may still be in use so defer to command queue to release it
	if (m_lastUsedCommandQueue)
	{
		m_lastUsedCommandQueue->AddDeferredResourceRelease(m_d3d12Resource, m_lastUsedFenceValue);
	}

	else
	{
		CommandQueue* commandQueue = m_renderer->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
		commandQueue->QueueUnMarkedResourceForDeletion(m_d3d12Resource);
	}

	m_d3d12Resource = nullptr;
	m_lastUsedCommandQueue = nullptr;
	m_lastUsedFenceValue = 0;
}

void ResourceDX12::SetD3D12Resource(ID3D12Resource* d3d12Resource, const D3D12_CLEAR_VALUE* clearValue)
{
	//Mark old resource for release
	if (m_d3d12Resource)
	{
		if (m_lastUsedCommandQueue)
		{
			m_lastUsedCommandQueue->AddDeferredResourceRelease(m_d3d12Resource, m_lastUsedFenceValue);
		}

		else
		{
			CommandQueue* commandQueue = m_renderer->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
			commandQueue->QueueUnMarkedResourceForDeletion(m_d3d12Resource);
		}

	}

	m_d3d12Resource = d3d12Resource;
	if (m_d3d12ClearValue)
	{
		m_d3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
	}
	else
	{
		m_d3d12ClearValue.reset();
	}
	SetName(m_resourceName);
}

D3D12_CPU_DESCRIPTOR_HANDLE ResourceDX12::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
	UNUSED(srvDesc);
	ERROR_AND_DIE(Stringf("Should not be calling ResourceDX12::GetShaderResourceView() Resource: %s", m_resourceName.c_str()));
}

D3D12_CPU_DESCRIPTOR_HANDLE ResourceDX12::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
	UNUSED(uavDesc);
	ERROR_AND_DIE(Stringf("Should not be calling ResourceDX12::GetUnorderedAccessView() Resource: %s", m_resourceName.c_str()));
}

void ResourceDX12::SetName(std::string const& name)
{
	m_resourceName = name;
	if (m_d3d12Resource && !m_resourceName.empty())
	{
		SetResourceName(m_d3d12Resource, name);
	}
}

void ResourceDX12::Reset()
{
	DX_SAFE_RELEASE(m_d3d12Resource);
	m_d3d12ClearValue.reset();
	m_FormatSupport = {};
}

bool ResourceDX12::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const
{
	return (m_FormatSupport.Support1 & formatSupport) != 0;
}

bool ResourceDX12::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const
{
	return (m_FormatSupport.Support2 & formatSupport) != 0;
}

void ResourceDX12::CheckFeatureSupport()
{
	if (m_d3d12Resource)
	{
		auto desc = m_d3d12Resource->GetDesc();
		auto device = m_renderer->GetDevice();

		m_FormatSupport.Format = desc.Format;
		HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &m_FormatSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT));
		GUARANTEE_OR_DIE(SUCCEEDED(hr), Stringf("Could not check feature support on Resource: %s", m_resourceName.c_str()));

	}
	else
	{
		m_FormatSupport = {};
	}
}

std::size_t ResourceDX12::HashSRVDesc(D3D12_SHADER_RESOURCE_VIEW_DESC const& srvDesc) const
{
	std::size_t hash = 0;
	auto hash_combine = [&hash](auto value) {
		hash ^= std::hash<std::decay_t<decltype(value)>>{}(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		};

	hash_combine(srvDesc.Format);
	hash_combine(srvDesc.ViewDimension);
	hash_combine(srvDesc.Shader4ComponentMapping);

	switch (srvDesc.ViewDimension)
	{
	case D3D12_SRV_DIMENSION_TEXTURE2D:
		hash_combine(srvDesc.Texture2D.MostDetailedMip);
		hash_combine(srvDesc.Texture2D.MipLevels);
		hash_combine(srvDesc.Texture2D.PlaneSlice);
		hash_combine(srvDesc.Texture2D.ResourceMinLODClamp);
		break;
		//#TODO: add more support for 3D textures
	default:
		break;
	}
	return hash;
}

std::size_t ResourceDX12::HashUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC const& uavDesc) const
{
	std::size_t hash = 0;
	auto hash_combine = [&hash](auto value) {
		hash ^= std::hash<std::decay_t<decltype(value)>>{}(value)+0x9e3779b9 + (hash << 6) + (hash >> 2);
		};

	hash_combine(uavDesc.Format);
	hash_combine(uavDesc.ViewDimension);

	switch (uavDesc.ViewDimension)
	{
	case D3D12_UAV_DIMENSION_TEXTURE2D:
		hash_combine(uavDesc.Texture2D.MipSlice);
		hash_combine(uavDesc.Texture2D.PlaneSlice);
		break;
	case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
		hash_combine(uavDesc.Texture2DArray.MipSlice);
		hash_combine(uavDesc.Texture2DArray.FirstArraySlice);
		hash_combine(uavDesc.Texture2DArray.ArraySize);
		hash_combine(uavDesc.Texture2DArray.PlaneSlice);
		break;

	case D3D12_UAV_DIMENSION_BUFFER:
		hash_combine(uavDesc.Buffer.FirstElement);
		hash_combine(uavDesc.Buffer.NumElements);
		hash_combine(uavDesc.Buffer.StructureByteStride);
		hash_combine(uavDesc.Buffer.CounterOffsetInBytes);
		hash_combine(uavDesc.Buffer.Flags);
		break;
	default:
		break;
	}
	return hash;
}

void ResourceDX12::MarkUsed(CommandQueue* commandQueueThatUsed, uint64_t const& fenceValue)
{
	if(!commandQueueThatUsed)
		return;

	m_lastUsedCommandQueue = commandQueueThatUsed;
	m_lastUsedFenceValue = fenceValue;
}
