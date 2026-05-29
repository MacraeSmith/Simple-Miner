#pragma once
#include "Engine/Renderer/ThreadSafeQueue.hpp"
#include <d3d12.h>
#include <cstdint> 
#include <string>
#include <queue>
#include <cstdint> 
#include <memory>
#include <atomic>  
#include <thread>
#include <mutex>
#include <condition_variable>


class CommandList;
class RendererDX12;
class ResourceDX12;
struct DeferredResourceRelease
{
	ID3D12Resource* m_resource = nullptr;
	uint64_t m_fenceValue = 0;
};

constexpr unsigned int MAX_NUM_COMMAND_LISTS = 5;
class CommandQueue
{
public:
	CommandQueue(RendererDX12* renderer, D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandQueue();

	CommandList* GetCommandList();
	ID3D12CommandQueue* GetD3D12CommandQueue() const;

	// Execute a command list. Returns the fence value to wait for for this command list.
	uint64_t		ExecuteCommandList(CommandList* commandList);
	uint64_t		ExecuteCommandLists(std::vector<CommandList*> const& commandLists);

	uint64_t		Signal();
	bool			IsFenceComplete(uint64_t fenceValue);
	void			WaitForFenceValue(uint64_t fenceValue);
	void			Flush();
	void			Wait(CommandQueue const& other);
	void			QueueResourceForMarkedUse(ResourceDX12* resource);
	void			AddDeferredResourceRelease(ID3D12Resource* resource, uint64_t lastFenceUsed);
	void			QueueUnMarkedResourceForDeletion(ID3D12Resource* resource);


protected:


private:
	void ProcessInFlightCommandLists();
	void InitializeCommandLists(std::string const& name);
	void TryToReleaseDeferredResources(uint64_t const& completedFenceValue);
	void MarkAllQueuedResources(uint64_t const& fenceValue);

public:
	std::string m_name;
private:
	// Keep track of command allocators that are "in-flight"
	struct CommandAllocatorEntry
	{
		uint64_t fenceValue;
		ID3D12CommandAllocator* commandAllocator;
	};


	using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;

	D3D12_COMMAND_LIST_TYPE                     m_commandListType;
	RendererDX12*							m_renderer = nullptr;
	ID3D12CommandQueue*							m_d3d12CommandQueue = nullptr;
	ID3D12Fence*								m_d3d12Fence = nullptr;
	std::atomic_uint64_t                        m_fenceValue = 0;

	std::vector<DeferredResourceRelease> m_deferredResourceReleases;
	std::vector<ResourceDX12*> m_resourcesQueuedForMarkedUse;


	using CommandListEntry = std::tuple<uint64_t, CommandList* >;
	ThreadSafeQueue<CommandListEntry>               m_InFlightCommandLists;
	ThreadSafeQueue<CommandList*>					m_AvailableCommandLists;
	

	//Thread safe command list organization
	std::thread				m_ProcessInFlightCommandListsThread;
	std::atomic_bool		m_bProcessInFlightCommandLists = false;
	std::mutex				m_ProcessInFlightCommandListsThreadMutex;
	std::condition_variable m_ProcessInFlightCommandListsThreadCV;
};

