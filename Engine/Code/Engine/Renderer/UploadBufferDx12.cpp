#include "Engine/Renderer/UploadBufferDX12.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include <new> // for std::bad_alloc

UploadBufferDX12::UploadBufferDX12(RendererDX12 const* renderer, size_t pageSize)
	:m_renderer(renderer)
	,m_pageSize(pageSize)
{
}

UploadBufferDX12::~UploadBufferDX12()
{
	// Explicitly clear the page lists before destruction
	m_currentPage.reset();

	for (auto& page : m_pagePool)
	{
		page.reset(); // triggers ~Page(), which releases m_dx12Resource
	}
	m_pagePool.clear();

	for (auto& page : m_availablePages)
	{
		page.reset();
	}
	m_availablePages.clear();

	m_renderer = nullptr;
}

Allocation UploadBufferDX12::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (sizeInBytes > m_pageSize)
	{
		throw std::bad_alloc();
	}

	// If there is no current page, or the requested allocation exceeds the
   // remaining space in the current page, request a new page.
	if (!m_currentPage || !m_currentPage->HasSpace(sizeInBytes, alignment))
	{
		m_currentPage = RequestPage();
	}

	return m_currentPage->Allocate(sizeInBytes, alignment);
}

void UploadBufferDX12::Reset()
{
	m_currentPage = nullptr;
	// Reset all available pages.
	m_availablePages = m_pagePool;

	for (auto page : m_availablePages)
	{
		// Reset the page for new allocations.
		page->Reset();
	}
}

std::shared_ptr<Page> UploadBufferDX12::RequestPage()
{
	std::shared_ptr<Page> page;

	if (!m_availablePages.empty())
	{
		page = m_availablePages.front();
		m_availablePages.pop_front();
	}
	else
	{
		page = std::make_shared<Page>(m_renderer, m_pageSize);
		m_pagePool.push_back(page);
	}

	return page;
}

//Page
//---------------------------------------------------------------------------------
Page::Page(RendererDX12 const* renderer, size_t sizeInBytes)
	:m_pageSize(sizeInBytes)
	,m_offset(0)
	,m_cpuPtr(nullptr)
	,m_gpuPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
	,m_renderer(renderer)
{
	ID3D12Device* device = m_renderer->GetDevice();

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = m_pageSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, 
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_dx12Resource));

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not created commited resource");

	m_gpuPtr = m_dx12Resource->GetGPUVirtualAddress();
	m_dx12Resource->Map(0, nullptr, &m_cpuPtr);
	SetResourceName(m_dx12Resource, "Added Page");
}

Page::~Page()
{
	m_dx12Resource->Unmap(0, nullptr);
	m_cpuPtr = nullptr;
	m_gpuPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
	DX_SAFE_RELEASE(m_dx12Resource);
}

bool Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
	size_t alignedSize = AlignUp(sizeInBytes, alignment);
	size_t alignedOffset = AlignUp(m_offset, alignment);

	return alignedOffset + alignedSize <= m_pageSize;
}

Allocation Page::Allocate(size_t sizeInBytes, size_t alignment)
{
	//may need std::lock_guard here to ensure thread safety

	if (!HasSpace(sizeInBytes, alignment))
	{
		// Can't allocate space from page.
		throw std::bad_alloc();
	}

	size_t alignedSize = AlignUp(sizeInBytes, alignment);
	m_offset = AlignUp(m_offset, alignment);

	Allocation allocation;
	allocation.CPU = static_cast<uint8_t*>(m_cpuPtr) + m_offset;
	allocation.GPU = m_gpuPtr + m_offset;

	m_offset += alignedSize;

	return allocation;
}

void Page::Reset()
{
	m_offset = 0;
}

