#pragma once

#include <d3d12.h>
#include <deque>
#include <wrl.h>
#include <memory>

class RendererDX12;

struct Allocation
{
	void* CPU;
	D3D12_GPU_VIRTUAL_ADDRESS GPU;
};

class Page
{
public:
	Page(RendererDX12 const* renderer, size_t sizeInBytes);
	~Page();

	bool		HasSpace(size_t sizeInBytes, size_t alignment) const;
	Allocation	Allocate(size_t sizeInBytes, size_t alignment);
	void		Reset();

private:
	ID3D12Resource* m_dx12Resource = nullptr;

	//base pointer
	void* m_cpuPtr;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuPtr;

	size_t  m_pageSize;
	size_t m_offset;
	RendererDX12 const* m_renderer = nullptr;
};



class UploadBufferDX12
{
public:
	explicit UploadBufferDX12(RendererDX12 const* renderer, size_t pageSize = 2097152); //2 mb
	~UploadBufferDX12();

	size_t		GetPageSize() const {return m_pageSize;}
	Allocation	Allocate(size_t sizeInBytes, size_t alignment);
	void		Reset();

private:
	size_t m_pageSize = 0;
	std::shared_ptr<Page> RequestPage();
	std::shared_ptr<Page> m_currentPage;
	using PagePool = std::deque<std::shared_ptr<Page>>;
	PagePool m_pagePool;
	PagePool m_availablePages;
	RendererDX12 const* m_renderer = nullptr;
};

