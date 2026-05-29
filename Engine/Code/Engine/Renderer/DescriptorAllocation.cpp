#include "Engine/Renderer/DescriptorAllocation.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include <algorithm>
#include <cassert>


//Descriptor Allocator
//--------------------------------------------------------------------------------------------------------------
DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, RendererDX12 const* renderer, uint32_t numDescriptorsPerHeap)
	:m_heapType(type)
	,m_renderer(renderer)
	,m_numDescriptorsPerHeap(numDescriptorsPerHeap)
{
}

DescriptorAllocator::~DescriptorAllocator()
{
	for (int i = 0; i < (int)m_heapPool.size(); ++i)
	{
		m_heapPool[i].reset();
	}
}

DescriptorAllocation DescriptorAllocator::Allocate(uint32_t numDescriptors)
{
	std::lock_guard<std::mutex> lock(m_allocationMutex);

	DescriptorAllocation allocation;

	for (auto iter = m_availableHeaps.begin(); iter != m_availableHeaps.end(); ++iter)
	{
		auto allocatorPage = m_heapPool[*iter];

		allocation = allocatorPage->Allocate(numDescriptors);

		if (allocatorPage->GetNumFreeHandles() == 0)
		{
			iter = m_availableHeaps.erase(iter);
		}

		// A valid allocation has been found.
		if (!allocation.IsNull())
		{
			break;
		}
	}

	// No available heap could satisfy the requested number of descriptors.
	if (allocation.IsNull())
	{
		//m_numDescriptorsPerHeap = std::max(m_numDescriptorsPerHeap, numDescriptors);
		m_numDescriptorsPerHeap = m_numDescriptorsPerHeap >= numDescriptors ? m_numDescriptorsPerHeap : numDescriptors;
		auto newPage = CreateAllocatorPage();

		allocation = newPage->Allocate(numDescriptors);
	}

	return allocation;
}

void DescriptorAllocator::ReleaseDescriptors(uint64_t frameNumber)
{
	std::lock_guard<std::mutex> lock(m_allocationMutex);

	for (size_t i = 0; i < m_heapPool.size(); ++i)
	{
		auto page = m_heapPool[i];

		page->ReleaseStaleDescriptors(frameNumber);

		if (page->GetNumFreeHandles() > 0)
		{
			m_availableHeaps.insert(i);
		}
	}
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
{
	auto newPage = std::make_shared<DescriptorAllocatorPage>(m_heapType, m_numDescriptorsPerHeap, m_renderer);

	m_heapPool.emplace_back(newPage);
	m_availableHeaps.insert(m_heapPool.size() - 1);

	return newPage;
}

//Descriptor Allocator Page
//--------------------------------------------------------------------------------------------------------------
DescriptorAllocatorPage::DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, RendererDX12 const* renderer)
	:m_heapType(type)
	,m_numDescriptorsInHeap(numDescriptors)
	,m_renderer(renderer)
{
	ID3D12Device* device = m_renderer->GetDevice();
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = m_heapType;
	heapDesc.NumDescriptors = m_numDescriptorsInHeap;

	HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create descriptor heap");

	m_baseDescriptor = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_descriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(m_heapType);
	m_numFreeHandles = m_numDescriptorsInHeap;

	// Initialize the free lists
	AddNewBlock(0, m_numFreeHandles);
}

DescriptorAllocatorPage::~DescriptorAllocatorPage()
{
	DX_SAFE_RELEASE(m_descriptorHeap);
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorAllocatorPage::GetHeapType() const
{
	return m_heapType;
}

uint32_t DescriptorAllocatorPage::GetNumFreeHandles() const
{
	return m_numFreeHandles;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocatorPage::GetDescriptorHandle(uint32_t index) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_baseDescriptor;
	handle.ptr += index * m_descriptorHandleIncrementSize;
	return handle;
}

bool DescriptorAllocatorPage::HasSpace(uint32_t numDescriptors) const
{
	return m_freeListBySize.lower_bound(numDescriptors) != m_freeListBySize.end();
}

void DescriptorAllocatorPage::Free(DescriptorAllocation&& descriptor, uint64_t frameNumber)
{
	// Compute the offset of the descriptor within the descriptor heap.
	auto offset = ComputeOffset(descriptor.GetCPUDescriptorHandle());

	std::lock_guard<std::mutex> lock(m_allocationMutex);

	// Don't add the block directly to the free list until the frame has completed.
	m_staleDescriptors.emplace(offset, descriptor.GetNumHandles(), frameNumber);
}

void DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t frameNumber)
{
	std::lock_guard<std::mutex> lock(m_allocationMutex);

	while (!m_staleDescriptors.empty() && m_staleDescriptors.front().m_frameNumber <= frameNumber)
	{
		auto& staleDescriptor = m_staleDescriptors.front();

		// The offset of the descriptor in the heap.
		auto offset = staleDescriptor.m_offset;
		// The number of descriptors that were allocated.
		auto numDescriptors = staleDescriptor.m_size;

		FreeBlock(offset, numDescriptors);

		m_staleDescriptors.pop();
	}
}

DescriptorAllocation DescriptorAllocatorPage::Allocate(uint32_t numDescriptors)
{
	std::lock_guard<std::mutex> lock(m_allocationMutex);
	// There are less than the requested number of descriptors left in the heap.
   // Return a NULL descriptor and try another heap.
	if (numDescriptors > m_numFreeHandles)
	{
		return DescriptorAllocation();
	}

	// Get the first block that is large enough to satisfy the request.
	auto smallestBlockIt = m_freeListBySize.lower_bound(numDescriptors);
	if (smallestBlockIt == m_freeListBySize.end())
	{
		// There was no free block that could satisfy the request.
		return DescriptorAllocation();
	}

	// The size of the smallest block that satisfies the request.
	auto blockSize = smallestBlockIt->first;

	// The pointer to the same entry in the FreeListByOffset map.
	auto offsetIt = smallestBlockIt->second;

	// The offset in the descriptor heap.
	auto offset = offsetIt->first;

	// Remove the existing free block from the free list.
	m_freeListBySize.erase(smallestBlockIt);
	m_freeListByOffset.erase(offsetIt);

	// Compute the new free block that results from splitting this block.
	auto newOffset = offset + numDescriptors;
	auto newSize = blockSize - numDescriptors;

	if (newSize > 0)
	{
		// If the allocation didn't exactly match the requested size,
		// return the left-over to the free list.
		AddNewBlock(newOffset, newSize);
	}
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_baseDescriptor;
	handle.ptr += static_cast<SIZE_T>(offset) * m_descriptorHandleIncrementSize;

	return DescriptorAllocation(
		handle,
		numDescriptors,
		m_descriptorHandleIncrementSize,
		shared_from_this());
}

uint32_t DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	return static_cast<uint32_t>(handle.ptr - m_baseDescriptor.ptr) / m_descriptorHandleIncrementSize;
}

void DescriptorAllocatorPage::AddNewBlock(uint32_t offset, uint32_t numDescriptors)
{
	auto offsetIt = m_freeListByOffset.emplace(offset, numDescriptors);
	auto sizeIt = m_freeListBySize.emplace(numDescriptors, offsetIt.first);
	offsetIt.first->second.m_freeListBySizeIterator = sizeIt;
}

void DescriptorAllocatorPage::FreeBlock(uint32_t offset, uint32_t numDescriptors)
{
	// Find the first element whose offset is greater than the specified offset.
	// This is the block that should appear after the block that is being freed.
	auto nextBlockIt = m_freeListByOffset.upper_bound(offset);

	// Find the block that appears before the block being freed.
	auto prevBlockIt = nextBlockIt;
	// If it's not the first block in the list.
	if (prevBlockIt != m_freeListByOffset.begin())
	{
		// Go to the previous block in the list.
		--prevBlockIt;
	}
	else
	{
		// Otherwise, just set it to the end of the list to indicate that no
		// block comes before the one being freed.
		prevBlockIt = m_freeListByOffset.end();
	}

	// Add the number of free handles back to the heap.
	// This needs to be done before merging any blocks since merging
	// blocks modifies the numDescriptors variable.
	m_numFreeHandles += numDescriptors;

	if (prevBlockIt != m_freeListByOffset.end() &&
		offset == prevBlockIt->first + prevBlockIt->second.m_size)
	{
		// The previous block is exactly behind the block that is to be freed.
		//
		// PrevBlock.Offset           Offset
		// |                          |
		// |<-----PrevBlock.Size----->|<------Size-------->|
		//

		// Increase the block size by the size of merging with the previous block.
		offset = prevBlockIt->first;
		numDescriptors += prevBlockIt->second.m_size;

		// Remove the previous block from the free list.
		m_freeListBySize.erase(prevBlockIt->second.m_freeListBySizeIterator);
		m_freeListByOffset.erase(prevBlockIt);
	}

	if (nextBlockIt != m_freeListByOffset.end() &&
		offset + numDescriptors == nextBlockIt->first)
	{
		// The next block is exactly in front of the block that is to be freed.
		//
		// Offset               NextBlock.Offset 
		// |                    |
		// |<------Size-------->|<-----NextBlock.Size----->|

		// Increase the block size by the size of merging with the next block.
		numDescriptors += nextBlockIt->second.m_size;

		// Remove the next block from the free list.
		m_freeListBySize.erase(nextBlockIt->second.m_freeListBySizeIterator);
		m_freeListByOffset.erase(nextBlockIt);
	}

	// Add the freed block to the free list.
	AddNewBlock(offset, numDescriptors);
}

//Descriptor Allocation
//--------------------------------------------------------------------------------------------------------------
DescriptorAllocation::DescriptorAllocation()
	:m_Descriptor{ 0 }
	,m_NumHandles(0)
	,m_DescriptorSize(0)
	,m_Page(nullptr)
{
}

DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t numHandles, uint32_t descriptorSize, std::shared_ptr<DescriptorAllocatorPage> page)
	: m_Descriptor(descriptor)
	, m_NumHandles(numHandles)
	, m_DescriptorSize(descriptorSize)
	, m_Page(page)
{
}

DescriptorAllocation::~DescriptorAllocation()
{
	Free();
}

DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& allocation)
	: m_Descriptor(allocation.m_Descriptor)
	, m_NumHandles(allocation.m_NumHandles)
	, m_DescriptorSize(allocation.m_DescriptorSize)
	, m_Page(std::move(allocation.m_Page))
{
	allocation.m_Descriptor.ptr = 0;
	allocation.m_NumHandles = 0;
	allocation.m_DescriptorSize = 0;
}

DescriptorAllocation& DescriptorAllocation::operator=(DescriptorAllocation&& other)
{
	// Free this descriptor if it points to anything.
	Free();

	m_Descriptor = other.m_Descriptor;
	m_NumHandles = other.m_NumHandles;
	m_DescriptorSize = other.m_DescriptorSize;
	m_Page = std::move(other.m_Page);

	other.m_Descriptor.ptr = 0;
	other.m_NumHandles = 0;
	other.m_DescriptorSize = 0;

	return *this;
}

void DescriptorAllocation::Free()
{
	if (!IsNull() && m_Page)
	{
		m_Page->Free(std::move(*this), Clock::GetSystemClock().GetFrameCount());

		m_Descriptor.ptr = 0;
		m_NumHandles = 0;
		m_DescriptorSize = 0;
		m_Page.reset();
	}
}

// Check if this a valid descriptor.
bool DescriptorAllocation::IsNull() const
{
	return m_Descriptor.ptr == 0;
}

// Get a descriptor at a particular offset in the allocation.
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetCPUDescriptorHandle(uint32_t offset) const
{
	assert(offset < m_NumHandles);
	return { m_Descriptor.ptr + (m_DescriptorSize * offset) };
}

uint32_t DescriptorAllocation::GetNumHandles() const
{
	return m_NumHandles;
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocation::GetDescriptorAllocatorPage() const
{
	return m_Page;
}

StaleDescriptorInfo::StaleDescriptorInfo(OffsetType offset, SizeType size, uint64_t frame)
	: m_offset(offset)
	, m_size(size)
	, m_frameNumber(frame)
{
}
