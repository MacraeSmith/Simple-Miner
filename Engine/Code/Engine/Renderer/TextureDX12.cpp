#include "Engine/Renderer/TextureDX12.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/ResourceStateTracker.hpp"
#include "Engine/Renderer/DynamicDescriptorHeap.hpp"


D3D12_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc(const D3D12_RESOURCE_DESC& resDesc, unsigned int mipSlice, unsigned int arraySlice = 0, unsigned int planeSlice = 0)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = resDesc.Format;

	switch (resDesc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		if (resDesc.DepthOrArraySize > 1)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
			uavDesc.Texture1DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
			uavDesc.Texture1DArray.FirstArraySlice = arraySlice;
			uavDesc.Texture1DArray.MipSlice = mipSlice;
		}
		else
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavDesc.Texture1D.MipSlice = mipSlice;
		}
		break;
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (resDesc.DepthOrArraySize > 1)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
			uavDesc.Texture2DArray.FirstArraySlice = arraySlice;
			uavDesc.Texture2DArray.PlaneSlice = planeSlice;
			uavDesc.Texture2DArray.MipSlice = mipSlice;
		}
		else
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.PlaneSlice = planeSlice;
			uavDesc.Texture2D.MipSlice = mipSlice;
		}
		break;
	case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.WSize = resDesc.DepthOrArraySize - arraySlice;
		uavDesc.Texture3D.FirstWSlice = arraySlice;
		uavDesc.Texture3D.MipSlice = mipSlice;
		break;
	default:
		throw std::exception("Invalid resource dimension.");
	}

	return uavDesc;
}

TextureDX12::TextureDX12(RendererDX12* renderer, std::string const& name, D3D12_RESOURCE_DESC const& resourceDesc, D3D12_CLEAR_VALUE const* clearValue)
	:ResourceDX12(renderer, resourceDesc, clearValue, name)
	,m_nonConstRenderer(renderer)
{
	CreateViews();
}

TextureDX12::TextureDX12(RendererDX12* renderer, std::string const& name, ID3D12Resource* resource)
	:ResourceDX12(renderer, resource, name)
	,m_nonConstRenderer(renderer)
{
	CreateViews();
}

TextureDX12::TextureDX12(RendererDX12* renderer, std::string const& name)
	:ResourceDX12(renderer, name)
	,m_nonConstRenderer(renderer)
{
}

TextureDX12::~TextureDX12()
{
	m_nonConstRenderer = nullptr;
}

void TextureDX12::CreateViews()
{
	if (m_d3d12Resource)
	{
		ID3D12Device* d3d12Device = m_nonConstRenderer->GetDevice();

		D3D12_RESOURCE_DESC desc(m_d3d12Resource->GetDesc());

		// Create RTV
		if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)// && CheckRTVSupport())
		{
			m_RenderTargetView = m_nonConstRenderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			d3d12Device->CreateRenderTargetView(m_d3d12Resource, nullptr, m_RenderTargetView.GetCPUDescriptorHandle());
		}
		// Create DSV
		if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)// && CheckDSVSupport())
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			if (desc.Format == DXGI_FORMAT_R24G8_TYPELESS)       // <-- added check
				dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  // compatible typed format for DSV
			else
				dsvDesc.Format = desc.Format;

			//dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // typed format for DSV
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

			m_depthStencilView = m_nonConstRenderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			d3d12Device->CreateDepthStencilView(m_d3d12Resource, &dsvDesc, m_depthStencilView.GetCPUDescriptorHandle());
		}
		
		std::lock_guard<std::mutex> lock(m_ShaderResourceViewsMutex);
		std::lock_guard<std::mutex> guard(m_UnorderedAccessViewsMutex);

		m_ShaderResourceViews.clear();
		m_UnorderedAccessViews.clear();
	}
}

void TextureDX12::CreateBindlessSRV(ID3D12DescriptorHeap* bindlessHeap, uint32_t heapIndex, D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc)
{
	unsigned int descriptorIncrement = m_renderer->GetBindlessDescriptorIncrementSize();
	m_cpuDescriptorHandle = bindlessHeap->GetCPUDescriptorHandleForHeapStart();
	m_cpuDescriptorHandle.ptr += heapIndex * descriptorIncrement;

	D3D12_SHADER_RESOURCE_VIEW_DESC localDesc = {};
	if (!srvDesc)
	{
		localDesc.Format = GetD3D12ResourceDesc().Format;
		localDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		localDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		localDesc.Texture2D.MostDetailedMip = 0;
		localDesc.Texture2D.MipLevels = GetD3D12ResourceDesc().MipLevels;
	}
	
	else
	{
		localDesc = *srvDesc;
	}

	ID3D12Device* device = m_renderer->GetDevice();
	device->CreateShaderResourceView(GetD3D12Resource(), &localDesc, m_cpuDescriptorHandle);

	m_gpuDescriptorHandle = bindlessHeap->GetGPUDescriptorHandleForHeapStart();
	m_gpuDescriptorHandle.ptr += heapIndex * descriptorIncrement;
	m_bindlessIndex = heapIndex;
}

DescriptorAllocation TextureDX12::CreateShaderResourceView(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const
{
	ID3D12Device* device = m_nonConstRenderer->GetDevice();
	DescriptorAllocation srv = m_nonConstRenderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (srvDesc != nullptr)
	{
		device->CreateShaderResourceView(m_d3d12Resource, srvDesc, srv.GetCPUDescriptorHandle());
		return srv;
	}

	D3D12_RESOURCE_DESC resourceDesc = m_d3d12Resource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC defaultSrvDesc = {};
	defaultSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	defaultSrvDesc.Format = resourceDesc.Format;

	if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
	{
		defaultSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		defaultSrvDesc.Texture3D.MostDetailedMip = 0;
		defaultSrvDesc.Texture3D.MipLevels = 1;
		defaultSrvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
	}
	else
	{
		defaultSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		defaultSrvDesc.Texture2D.MostDetailedMip = 0;
		defaultSrvDesc.Texture2D.MipLevels = 1;
		defaultSrvDesc.Texture2D.PlaneSlice = 0;
		defaultSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}

	device->CreateShaderResourceView(m_d3d12Resource, &defaultSrvDesc, srv.GetCPUDescriptorHandle());
	return srv;
}

DescriptorAllocation TextureDX12::CreateUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc) const
{
	ID3D12Device* device = m_nonConstRenderer->GetDevice();
	DescriptorAllocation uav = m_nonConstRenderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (uavDesc != nullptr)
	{
		device->CreateUnorderedAccessView(m_d3d12Resource, nullptr, uavDesc, uav.GetCPUDescriptorHandle());
		return uav;
	}

	D3D12_RESOURCE_DESC resourceDesc = m_d3d12Resource->GetDesc();

	D3D12_UNORDERED_ACCESS_VIEW_DESC defaultUavDesc = {};
	defaultUavDesc.Format = resourceDesc.Format;

	if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
	{
		defaultUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		defaultUavDesc.Texture3D.MipSlice = 0;
		defaultUavDesc.Texture3D.FirstWSlice = 0;
		defaultUavDesc.Texture3D.WSize = GetDepth();
	}
	else
	{
		defaultUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		defaultUavDesc.Texture2D.MipSlice = 0;
		defaultUavDesc.Texture2D.PlaneSlice = 0;
	}

	device->CreateUnorderedAccessView(m_d3d12Resource, nullptr, &defaultUavDesc, uav.GetCPUDescriptorHandle());
	return uav;
}

void TextureDX12::Resize(IntVec2 const& dimensions, unsigned int depthOrArraySize)
{
	if (m_d3d12Resource)
	{
		D3D12_RESOURCE_DESC oldDesc = m_d3d12Resource->GetDesc();

		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = oldDesc.Dimension;
		resDesc.Alignment = oldDesc.Alignment;
		resDesc.Width = GetMax((unsigned int)dimensions.x, 1u);
		resDesc.Height = GetMax((unsigned int)dimensions.y, 1u);
		resDesc.DepthOrArraySize = (uint16_t)depthOrArraySize;
		resDesc.MipLevels = oldDesc.SampleDesc.Count > 1 ? 1 : 0;
		resDesc.Format = oldDesc.Format;
		resDesc.SampleDesc = oldDesc.SampleDesc;
		resDesc.Layout = oldDesc.Layout;
		resDesc.Flags = oldDesc.Flags;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		ID3D12Device* device = m_renderer->GetDevice();
		HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COMMON, m_d3d12ClearValue.get(), IID_PPV_ARGS(&m_d3d12Resource));
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create commited resource for texture");

		ResourceStateTracker::AddGlobalResourceState(m_d3d12Resource, D3D12_RESOURCE_STATE_COMMON);

		CreateViews();
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureDX12::GetRenderTargetView() const
{
	return m_RenderTargetView.GetCPUDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureDX12::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
	std::size_t hash = 0;
	if (srvDesc)
	{
		hash = HashSRVDesc(*srvDesc);
	}

	std::lock_guard<std::mutex> lock(m_ShaderResourceViewsMutex);

	auto iter = m_ShaderResourceViews.find(hash);
	if (iter == m_ShaderResourceViews.end())
	{
		auto srv = CreateShaderResourceView(srvDesc);
		iter = m_ShaderResourceViews.insert({ hash, std::move(srv) }).first;
	}

	return iter->second.GetCPUDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureDX12::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
	std::size_t hash = 0;
	if (uavDesc)
	{
		hash = HashUAVDesc(*uavDesc);
	}

	std::lock_guard<std::mutex> guard(m_UnorderedAccessViewsMutex);

	auto iter = m_UnorderedAccessViews.find(hash);
	if (iter == m_UnorderedAccessViews.end())
	{
		auto uav = CreateUnorderedAccessView(uavDesc);
		iter = m_UnorderedAccessViews.insert({ hash, std::move(uav) }).first;
	}

	return iter->second.GetCPUDescriptorHandle();
}


D3D12_CPU_DESCRIPTOR_HANDLE TextureDX12::GetDepthStencilView() const
{
	return m_depthStencilView.GetCPUDescriptorHandle();
}

bool TextureDX12::HasAlpha() const
{
	DXGI_FORMAT format = GetD3D12ResourceDesc().Format;

	bool hasAlpha = false;

	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
	case DXGI_FORMAT_A8P8:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		hasAlpha = true;
		break;
	}

	return hasAlpha;
}

bool TextureDX12::IsUAVCompatibleFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SINT:
		return true;
	default:
		return false;
	}
}

bool TextureDX12::IsSRGBFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return true;
	default:
		return false;
	}
}

bool TextureDX12::IsBGRFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return true;
	default:
		return false;
	}
}

bool TextureDX12::IsDepthFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_D16_UNORM:
		return true;
	default:
		return false;
	}
}

DXGI_FORMAT TextureDX12::GetTypelessFormat(DXGI_FORMAT format)
{
	DXGI_FORMAT typelessFormat = format;

	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		typelessFormat = DXGI_FORMAT_R32G32B32A32_TYPELESS;
		break;
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		typelessFormat = DXGI_FORMAT_R32G32B32_TYPELESS;
		break;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
		typelessFormat = DXGI_FORMAT_R16G16B16A16_TYPELESS;
		break;
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
		typelessFormat = DXGI_FORMAT_R32G32_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		typelessFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
		break;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
		typelessFormat = DXGI_FORMAT_R10G10B10A2_TYPELESS;
		break;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
		typelessFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
		break;
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
		typelessFormat = DXGI_FORMAT_R16G16_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
		typelessFormat = DXGI_FORMAT_R32_TYPELESS;
		break;
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
		typelessFormat = DXGI_FORMAT_R8G8_TYPELESS;
		break;
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
		typelessFormat = DXGI_FORMAT_R16_TYPELESS;
		break;
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
		typelessFormat = DXGI_FORMAT_R8_TYPELESS;
		break;
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC1_TYPELESS;
		break;
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC2_TYPELESS;
		break;
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC3_TYPELESS;
		break;
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		typelessFormat = DXGI_FORMAT_BC4_TYPELESS;
		break;
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
		typelessFormat = DXGI_FORMAT_BC5_TYPELESS;
		break;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
		break;
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_B8G8R8X8_TYPELESS;
		break;
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
		typelessFormat = DXGI_FORMAT_BC6H_TYPELESS;
		break;
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC7_TYPELESS;
		break;
	}

	return typelessFormat;
}

DXGI_FORMAT TextureDX12::GetSRGBFormat(DXGI_FORMAT format)
{
	DXGI_FORMAT srgbFormat = format;
	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		srgbFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC1_UNORM:
		srgbFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC2_UNORM:
		srgbFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC3_UNORM:
		srgbFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
		break;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		srgbFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		break;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		srgbFormat = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC7_UNORM:
		srgbFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
		break;
	}

	return srgbFormat;
}

DXGI_FORMAT TextureDX12::GetUAVCompatableFormat(DXGI_FORMAT format)
{
	DXGI_FORMAT uavFormat = format;

	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		uavFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
		uavFormat = DXGI_FORMAT_R32_FLOAT;
		break;
	}

	return uavFormat;
}

void TextureDX12::SetDimensions(IntVec2 const& dimensions)
{
	m_dimensions = dimensions;
}

uint16_t TextureDX12::GetMipLevelCount() const
{
	D3D12_RESOURCE_DESC const resourceDescription = m_d3d12Resource->GetDesc();
	return resourceDescription.MipLevels;
}
