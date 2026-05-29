#pragma once
#include <d3d12.h>
#include <cstdint>
#include <mutex>
#include <memory>
#include <set>
#include <vector>
#include <map>
#include <queue>

class RendererDX12;
class DescriptorAllocatorPage;

//Descriptor Allocation
//--------------------------------------------------------------------------------------------------------------
class DescriptorAllocation
{
public:
	DescriptorAllocation(); // Creates a NULL descriptor.
	DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t numHandles, uint32_t descriptorSize, std::shared_ptr<DescriptorAllocatorPage> page);
	~DescriptorAllocation();
	
	//Copies are not allowed
	DescriptorAllocation(const DescriptorAllocation&) = delete;
	DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

	// Move is allowed.
	DescriptorAllocation(DescriptorAllocation&& allocation);
	DescriptorAllocation& operator=(DescriptorAllocation&& other);

	// Check if this a valid descriptor.
	bool IsNull() const;

	// Get a descriptor at a particular offset in the allocation.
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t offset = 0) const;

	// Get the number of (consecutive) handles for this allocation.
	uint32_t GetNumHandles() const;

	// Get the heap that this allocation came from.
   // (For internal use only).
	std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage() const;


private:
	// Free the descriptor back to the heap it came from.
	void Free();

	// The base descriptor.
	D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptor;
	// The number of descriptors in this allocation.
	uint32_t m_NumHandles;
	// The offset to the next descriptor.
	uint32_t m_DescriptorSize;

	// A pointer back to the original page where this allocation came from.
	std::shared_ptr<DescriptorAllocatorPage> m_Page;
};

//Descriptor Allocator Page
//--------------------------------------------------------------------------------------------------------------


using OffsetType = uint32_t; // The offset(in descriptors) within the descriptor heap.
using SizeType = uint32_t; // The number of descriptors that are available.
struct FreeBlockInfo;
struct StaleDescriptorInfo;
using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;// A map that lists the free blocks by size. Needs to be a multimap since multiple blocks can have the same size.
using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;
using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

struct FreeBlockInfo
{
	FreeBlockInfo(SizeType size)
		: m_size(size)
	{}

	SizeType m_size;
	FreeListBySize::iterator m_freeListBySizeIterator;
};

struct StaleDescriptorInfo
{
	StaleDescriptorInfo(OffsetType offset, SizeType size, uint64_t frame);
	
	OffsetType	m_offset; // The offset within the descriptor heap.
	SizeType	m_size; // The number of descriptors
	uint64_t	m_frameNumber; // The frame number that the descriptor was freed.
};

class DescriptorAllocatorPage : public std::enable_shared_from_this<DescriptorAllocatorPage>
{
public:
	DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, RendererDX12 const* renderer);
	~DescriptorAllocatorPage();

	D3D12_DESCRIPTOR_HEAP_TYPE	GetHeapType() const;
	bool						HasSpace(uint32_t numDescriptors) const;
	uint32_t					GetNumFreeHandles() const;
	void						Free(DescriptorAllocation&& descriptor, uint64_t frameNumber);

	void						ReleaseStaleDescriptors(uint64_t frameNumber);
	DescriptorAllocation		Allocate(uint32_t numDescriptors);

protected:

	uint32_t					ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle); // Compute the offset of the descriptor handle from the start of the heap.
	void						AddNewBlock(uint32_t offset, uint32_t numDescriptors); // Adds a new block to the free list.
	void						FreeBlock(uint32_t offset, uint32_t numDescriptors); // Free a block of descriptors.This will also merge free blocks in the free list to form larger blocks that can be reused.

private:
	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t index) const;

	FreeListByOffset			m_freeListByOffset;
	FreeListBySize				m_freeListBySize;
	StaleDescriptorQueue		m_staleDescriptors;

	ID3D12DescriptorHeap*		m_descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE	m_heapType;
	D3D12_CPU_DESCRIPTOR_HANDLE m_baseDescriptor = {};
	uint32_t					m_numFreeHandles;
	
	uint32_t					m_descriptorHandleIncrementSize;
	uint32_t					m_numDescriptorsInHeap;

	std::mutex					m_allocationMutex;

	RendererDX12 const*			m_renderer = nullptr;
};


//Descriptor Allocator
//--------------------------------------------------------------------------------------------------------------
class DescriptorAllocator
{
public:
	DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, RendererDX12 const* renderer, uint32_t numDescriptorsPerHeap = 256 );
	virtual ~DescriptorAllocator();

	DescriptorAllocation		Allocate(uint32_t numDescriptors = 1);
	void						ReleaseDescriptors(uint64_t frameNumber);

private:
	std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage(); // Create a new heap with a specific number of descriptors.

private:
	using DescriptorHeapPool = std::vector< std::shared_ptr<DescriptorAllocatorPage> >;

	D3D12_DESCRIPTOR_HEAP_TYPE		m_heapType;
	uint32_t						m_numDescriptorsPerHeap;
	DescriptorHeapPool				m_heapPool;
	std::set<size_t>				m_availableHeaps; // Indices of available heaps in the heap pool.
	std::mutex						m_allocationMutex;
	RendererDX12 const*				m_renderer = nullptr;
};

