#include "Engine/Renderer/DynamicDescriptorHeap.hpp"
#include "Engine/Renderer/RootSignatureDX12.hpp"
#include "Engine/Renderer/CommandList.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <cassert>


DynamicDescriptorHeap::DynamicDescriptorHeap(RendererDX12 const* renderer, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptorsPerHeap)
	: m_renderer(renderer)
	, m_descriptorHeapType(heapType)
	, m_numDescriptorsPerHeap(numDescriptorsPerHeap)
	, m_descriptorTableBitMask(0)
	, m_staleDescriptorTableBitMask(0)
	, m_currentCPUDescriptorHandle{}
	, m_currentGPUDescriptorHandle{}
	, m_numFreeHandles(0)
{
	ID3D12Device* device = m_renderer->GetDevice();
	m_descriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(heapType);

	// Allocate space for staging CPU visible descriptors.
	m_descriptorHandleCache = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(m_numDescriptorsPerHeap);
}

DynamicDescriptorHeap::~DynamicDescriptorHeap()
{
#ifdef ENGINE_DEBUG_RENDERER

	if (m_descriptorHeapPool.size() > 0)
	{
		std::string heapTypeNames[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = { "CBV_SRV_UAV", "SAMPLER", "RTV", "DSV" };
		DebuggerPrintf("\n-----\n~DynamicDescriptorHeap() type=%s this=%p poolSize=%u\n-----\n",
			heapTypeNames[(int)m_descriptorHeapType].c_str(), this, (unsigned int)m_descriptorHeapPool.size());
	}
#endif // ENGINE_DEBUG_RENDERER

	while (!m_availableDescriptorHeaps.empty())
	{
		m_availableDescriptorHeaps.pop();
	}

	// The pool owns every heap ever created. Release them exactly once here.
	while (!m_descriptorHeapPool.empty())
	{
		ID3D12DescriptorHeap* heap = m_descriptorHeapPool.front();
		m_descriptorHeapPool.pop();
#ifdef ENGINE_DEBUG_RENDERER
		std::string heapTypeNames[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = { "CBV_SRV_UAV", "SAMPLER", "RTV", "DSV" };
		DebuggerPrintf("\n-----\nDYNAMIC DESCRIPTOR HEAP RELEASE type=%s heap=%p this=%p\n-----\n", heapTypeNames[(int)m_descriptorHeapType].c_str(), heap, this);
#endif
		DX_SAFE_RELEASE(heap);
	}

	// Do NOT release m_currentDescriptorHeap separately (it was released via the pool).
	m_currentDescriptorHeap = nullptr;

	/*
	while (!m_descriptorHeapPool.empty())
	{
		ID3D12DescriptorHeap* heap = m_descriptorHeapPool.front();

		if (heap != m_currentDescriptorHeap)
		{
			DX_SAFE_RELEASE(heap);
		}
		m_descriptorHeapPool.pop();
	}

	// Release any leftover available heaps
	while (!m_availableDescriptorHeaps.empty())
	{
		ID3D12DescriptorHeap* heap = m_availableDescriptorHeaps.front();

		if (heap != m_currentDescriptorHeap)
		{
			DX_SAFE_RELEASE(heap);
		}
		m_availableDescriptorHeaps.pop();
	}

	DX_SAFE_RELEASE(m_currentDescriptorHeap);
	*/
	
}

void DynamicDescriptorHeap::ParseRootSignature(RootSignatureDX12 const& rootSignature)
{
	// If the root signature changes, all descriptors must be (re)bound to the command list.
	m_staleDescriptorTableBitMask = 0;

	const auto& rootSignatureDesc = rootSignature.GetRootSignatureDesc();

	// Get a bit mask that represents the root parameter indices that match the descriptor heap type for this dynamic descriptor heap.
	m_descriptorTableBitMask = rootSignature.GetDescriptorTableBitMask(m_descriptorHeapType);

	uint32_t descriptorTableBitMask = m_descriptorTableBitMask;
	uint32_t currentOffset = 0;
	DWORD rootIndex;
	while (_BitScanForward(&rootIndex, descriptorTableBitMask) && rootIndex < rootSignatureDesc.NumParameters)
	{
		uint32_t numDescriptors = rootSignature.GetNumDescriptors(rootIndex);

		DescriptorTableCache& descriptorTableCache = m_descriptorTableCache[rootIndex];
		descriptorTableCache.NumDescriptors = numDescriptors;
		descriptorTableCache.BaseDescriptor = m_descriptorHandleCache.get() + currentOffset;

		currentOffset += numDescriptors;

		// Flip the descriptor table bit so it's not scanned again for the current index.
		descriptorTableBitMask ^= (1 << rootIndex);
	}

	// Make sure the maximum number of descriptors per descriptor heap has not been exceeded.
	assert(currentOffset <= m_numDescriptorsPerHeap && "The root signature requires more than the maximum number of descriptors per descriptor heap. Consider increasing the maximum number of descriptors per descriptor heap.");
}

void DynamicDescriptorHeap::StageDescriptors(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor)
{
	if (srcDescriptor.ptr == 0)
	{
		assert(false && "StageDescriptors: srcDescriptor.ptr is null! Descriptor was not created or is invalid.");
	}

	// Cannot stage more than the maximum number of descriptors per heap.
	// Cannot stage more than MaxDescriptorTables root parameters.
	if (numDescriptors > m_numDescriptorsPerHeap || rootParameterIndex >= MAX_DESCRIPTOR_TABLES)
	{
		throw std::bad_alloc();
	}

	DescriptorTableCache& descriptorTableCache = m_descriptorTableCache[rootParameterIndex];

	// Check that the number of descriptors to copy does not exceed the number of descriptors expected in the descriptor table.
	if ((offset + numDescriptors) > descriptorTableCache.NumDescriptors)
	{
		throw std::length_error("Number of descriptors exceeds the number of descriptors in the descriptor table.");
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = (descriptorTableCache.BaseDescriptor + offset);
	for (uint32_t i = 0; i < numDescriptors; ++i)
	{
		dstDescriptor[i].ptr = srcDescriptor.ptr + static_cast<SIZE_T>(i) * m_descriptorHandleIncrementSize;
	}

	// Set the root parameter index bit to make sure the descriptor table at that index is bound to the command list.
	m_staleDescriptorTableBitMask |= (1 << rootParameterIndex);
	
}

uint32_t DynamicDescriptorHeap::ComputeStaleDescriptorCount() const
{
	uint32_t numStaleDescriptors = 0;
	DWORD i;
	DWORD staleDescriptorsBitMask = m_staleDescriptorTableBitMask;

	while (_BitScanForward(&i, staleDescriptorsBitMask))
	{
		numStaleDescriptors += m_descriptorTableCache[i].NumDescriptors;
		staleDescriptorsBitMask ^= (1 << i);
	}

	return numStaleDescriptors;
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::RequestDescriptorHeap()
{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	if (!m_availableDescriptorHeaps.empty())
	{
		descriptorHeap = m_availableDescriptorHeaps.front();
		m_availableDescriptorHeaps.pop();
	}
	else
	{
		descriptorHeap = CreateDescriptorHeap();

		m_descriptorHeapPool.push(descriptorHeap);
#ifdef ENGINE_DEBUG_RENDERER
		DebuggerPrintf("\n-----\nDYNAMIC DESCRIPTOR HEAP POOL PUSH heap=%p poolSize=%u this=%p\n-----\n", 
			(int)m_descriptorHeapType, descriptorHeap, (unsigned int)m_descriptorHeapPool.size(), this);
#endif
	}

	return descriptorHeap;
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::CreateDescriptorHeap()
{
	ID3D12Device* device = m_renderer->GetDevice();

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = m_descriptorHeapType;
	descriptorHeapDesc.NumDescriptors = m_numDescriptorsPerHeap;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not create descriptor heap");

#ifdef ENGINE_DEBUG_RENDERER
	std::string heapTypeNames[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = { "CBV_SRV_UAV", "SAMPLER", "RTV", "DSV" };
	SetDescriptorHeapName(descriptorHeap, Stringf("DynamicDescriptorHeap::CreateDescriptorHeap() DescriptorHeap: %s", heapTypeNames[(int)m_descriptorHeapType].c_str()));
	DebuggerPrintf("\n-----\nDYNAMIC DESCRIPTOR HEAP CREATE type=%s heap=%p\n-----\n", heapTypeNames[(int)m_descriptorHeapType].c_str(), descriptorHeap);
#endif //ENGINE_DEBUG_RENDERER

	return descriptorHeap;
}

void DynamicDescriptorHeap::CommitStagedDescriptors(CommandList& commandList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc)
{
	// Compute the number of descriptors that need to be copied 
	uint32_t numDescriptorsToCommit = ComputeStaleDescriptorCount();
	if (numDescriptorsToCommit > 0)
	{
		ID3D12Device* device = m_renderer->GetDevice();
		ID3D12GraphicsCommandList* d3d12GraphicsCommandList = commandList.GetGraphicsCommandList();
 		assert(d3d12GraphicsCommandList != nullptr);

		if (!m_currentDescriptorHeap || m_numFreeHandles < numDescriptorsToCommit)
		{
			m_currentDescriptorHeap = RequestDescriptorHeap();

			m_currentCPUDescriptorHandle = m_currentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			m_currentGPUDescriptorHandle = m_currentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			m_numFreeHandles = m_numDescriptorsPerHeap;

			commandList.SetDescriptorHeap(m_descriptorHeapType, m_currentDescriptorHeap);

			// When updating the descriptor heap on the command list, all descriptor
			// tables must be (re)recopied to the new descriptor heap (not just
			// the stale descriptor tables).
			m_staleDescriptorTableBitMask = m_descriptorTableBitMask;
		}

		DWORD rootIndex;
		// Scan from LSB to MSB for a bit set in staleDescriptorsBitMask
		while (_BitScanForward(&rootIndex, m_staleDescriptorTableBitMask))
		{
			UINT numSrcDescriptors = m_descriptorTableCache[rootIndex].NumDescriptors;
			D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorHandles = m_descriptorTableCache[rootIndex].BaseDescriptor;

			bool nullDescriptor = false;
			for(UINT i = 0; i < numSrcDescriptors; ++i)
			{
				if (pSrcDescriptorHandles[i].ptr == 0)
				{
					nullDescriptor = true;
					DebuggerPrintf(Stringf("Null descriptor detected at index: %i for rootIndex: %i \n", i, rootIndex).c_str());
					break;
				}
			}

			// Flip the stale bit so the descriptor table is not recopied again unless it is updated with a new descriptor.
			m_staleDescriptorTableBitMask ^= (1 << rootIndex);

			if (nullDescriptor)
			{
				continue; //Protection against null descriptors, If binding is done correctly this should not occur
			}

			D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[] =
			{
				m_currentCPUDescriptorHandle
			};

			UINT pDestDescriptorRangeSizes[] = {numSrcDescriptors};

			// Copy the staged CPU visible descriptors to the GPU visible descriptor heap.
			device->CopyDescriptors(1, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
				numSrcDescriptors, pSrcDescriptorHandles, nullptr, m_descriptorHeapType);

			// Set the descriptors on the command list using the passed-in setter function.
			setFunc(d3d12GraphicsCommandList, rootIndex, m_currentGPUDescriptorHandle);

			// Offset current CPU and GPU descriptor handles.
			m_currentCPUDescriptorHandle.ptr += static_cast<SIZE_T>(numSrcDescriptors) * m_descriptorHandleIncrementSize;
			m_currentGPUDescriptorHandle.ptr += static_cast<SIZE_T>(numSrcDescriptors) * m_descriptorHandleIncrementSize;
			m_numFreeHandles -= numSrcDescriptors;
		}
	}
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDraw(CommandList& commandList)
{
	CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDispatch(CommandList& commandList)
{
	CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
}

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::CopyDescriptor(CommandList& comandList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor)
{
	if (!m_currentDescriptorHeap || m_numFreeHandles < 1)
	{
		m_currentDescriptorHeap = RequestDescriptorHeap();
		m_currentCPUDescriptorHandle = m_currentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_currentGPUDescriptorHandle = m_currentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_numFreeHandles = m_numDescriptorsPerHeap;

		comandList.SetDescriptorHeap(m_descriptorHeapType, m_currentDescriptorHeap);

		// When updating the descriptor heap on the command list, all descriptor
		// tables must be (re)recopied to the new descriptor heap (not just
		// the stale descriptor tables).
		m_staleDescriptorTableBitMask = m_descriptorTableBitMask;
	}

	ID3D12Device* device = m_renderer->GetDevice();

	D3D12_GPU_DESCRIPTOR_HANDLE hGPU = m_currentGPUDescriptorHandle;
	device->CopyDescriptorsSimple(1, m_currentCPUDescriptorHandle, cpuDescriptor, m_descriptorHeapType);

	m_currentCPUDescriptorHandle.ptr += static_cast<SIZE_T>(m_descriptorHandleIncrementSize);
	m_currentGPUDescriptorHandle.ptr += static_cast<UINT64>(m_descriptorHandleIncrementSize);
	m_numFreeHandles -= 1;

	return hGPU;
}

void DynamicDescriptorHeap::Reset()
{
	if (m_currentDescriptorHeap)
	{
		m_availableDescriptorHeaps.push(m_currentDescriptorHeap);
		m_currentDescriptorHeap = nullptr;
	}

	m_currentCPUDescriptorHandle.ptr = 0;
	m_currentGPUDescriptorHandle.ptr = 0;
	m_numFreeHandles = 0;
	m_descriptorTableBitMask = 0;
	m_staleDescriptorTableBitMask = 0;

	for (int i = 0; i < MAX_DESCRIPTOR_TABLES; ++i)
	{
		m_descriptorTableCache[i].Reset();
	}

	/*
	m_availableDescriptorHeaps = std::move(m_descriptorHeapPool);
	//#TODO: figure out why I am getting one descriptor heap memory leak or error if I add bind texture
	//destroying all descriptor allocations and descriptor allocation pages seemed to fix this


// 	if (m_currentDescriptorHeap)
// 		DX_SAFE_RELEASE(m_currentDescriptorHeap);

	if (m_currentDescriptorHeap)
	{
		m_availableDescriptorHeaps.push(m_currentDescriptorHeap);
		m_currentDescriptorHeap = nullptr;
	}

	m_currentCPUDescriptorHandle.ptr = 0;
	m_currentGPUDescriptorHandle.ptr = 0;
	m_numFreeHandles = 0;
	m_descriptorTableBitMask = 0;
	m_staleDescriptorTableBitMask = 0;

	// Reset the table cache
	for (int i = 0; i < MAX_DESCRIPTOR_TABLES; ++i)
	{
		m_descriptorTableCache[i].Reset();
	}
	*/
}


