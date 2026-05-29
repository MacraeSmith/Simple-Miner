#pragma once
#include <d3d12.h>

#include <mutex>
#include <map>
#include <unordered_map>
#include <vector>

class CommandList;
class ResourceDX12;

// Tracks the state of a particular resource and all of its subresources.
struct ResourceState
{
	// Initialize all of the subresources within a resource to the given state.
	explicit ResourceState(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON)
		: State(state)
	{}
	// Set a subresource to a particular state.
	void SetSubresourceState(unsigned int subresource, D3D12_RESOURCE_STATES state)
	{
		if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
		{
			State = state;
			SubresourceState.clear();
		}
		else
		{
			SubresourceState[subresource] = state;
		}
	}
	// Get the state of a (sub)resource within the resource.
// If the specified subresource is not found in the SubresourceState array (map)
// then the state of the resource (D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) is
// returned.
	D3D12_RESOURCE_STATES GetSubresourceState(unsigned int subresource) const
	{
		D3D12_RESOURCE_STATES state = State;
		const auto iter = SubresourceState.find(subresource);
		if (iter != SubresourceState.end())
		{
			state = iter->second;
		}
		return state;
	}
	// If the SubresourceState array (map) is empty, then the State variable defines 
// the state of all of the subresources.
	D3D12_RESOURCE_STATES State;
	std::map<unsigned int, D3D12_RESOURCE_STATES> SubresourceState;
};

class ResourceStateTracker
{
public:
	ResourceStateTracker() {};
	virtual ~ResourceStateTracker();
	#pragma region Explanation...
    /**
 * Push a resource barrier to the resource state tracker.
 *
 * @param barrier The resource barrier to push to the resource state tracker.
 */
    #pragma endregion
    void ResourceBarrier(D3D12_RESOURCE_BARRIER const& barrier);

	#pragma region Explanation...
    /**
 * Push a transition resource barrier to the resource state tracker.
 *
 * @param resource The resource to transition.
 * @param stateAfter The state to transition the resource to.
 * @param subResource The subresource to transition. By default, this is D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
 * which indicates that all subresources should be transitioned to the same state.
 */
    #pragma endregion
    void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, unsigned int subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    void TransitionResource(ResourceDX12 const& resource, D3D12_RESOURCE_STATES stateAfter, unsigned int subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	#pragma region Explanation...
    /**
 * Push a UAV resource barrier for the given resource.
 *
 * @param resource The resource to add a UAV barrier for. Can be NULL which
 * indicates that any UAV access could require the barrier.
 */
    #pragma endregion
    void UAVBarrier(ResourceDX12 const* resource = nullptr);
   
	#pragma region Explanation...
    /**
 * Push an aliasing barrier for the given resource.
 *
 * @param beforeResource The resource currently occupying the space in the heap.
 * @param afterResource The resource that will be occupying the space in the heap.
 *
 * Either the beforeResource or the afterResource parameters can be NULL which
 * indicates that any placed or reserved resource could cause aliasing.
 */
    #pragma endregion
    void AliasBarrier(ResourceDX12 const* resourceBefore = nullptr, ResourceDX12 const* resourceAfter = nullptr);
	#pragma region Explanation...
    /**
 * Flush any pending resource barriers to the command list.
 *
 * @return The number of resource barriers that were flushed to the command list.
 */
    #pragma endregion
    uint32_t FlushPendingResourceBarriers(CommandList& commandList);
   
	#pragma region Explanation...
    /**
 * Flush any (non-pending) resource barriers that have been pushed to the resource state
 * tracker.
 */
    #pragma endregion
    void FlushResourceBarriers(CommandList& commandList);
   
	#pragma region Explanation...
    /**
 * Commit final resource states to the global resource state map.
 * This must be called when the command list is closed.
 */
    #pragma endregion
    void CommitFinalResourceStates();
    
	#pragma region Explanation...
    /**
 * Reset state tracking. This must be done when the command list is reset.
 */
    #pragma endregion
    void Reset();

	D3D12_RESOURCE_STATES GetResourceState(ID3D12Resource* resource, unsigned int subresource) const;

	#pragma region Explanation...
    /**
 * The global state must be locked before flushing pending resource barriers
 * and committing the final resource state to the global resource state.
 * This ensures consistency of the global resource state between command list
 * executions.
 */
	#pragma endregion
    static void Lock();
	#pragma region Explanation...
    /**
     * Unlocks the global resource state after the final states have been committed
     * to the global resource state array.
     */
#pragma endregion
    static void Unlock();
	#pragma region Explanation...
    /**
 * Add a resource with a given state to the global resource state array (map).
 * This should be done when the resource is created for the first time.
 */
#pragma endregion
    static void AddGlobalResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state);
	#pragma region Explanation...
    /**
     * Remove a resource from the global resource state array (map).
     * This should only be done when the resource is destroyed.
     */
#pragma endregion
    static void RemoveGlobalResourceState(ID3D12Resource* resource);



private:
	// An array (vector) of resource barriers.
	using ResourceBarriers = std::vector<D3D12_RESOURCE_BARRIER>;
	#pragma region Explanation...
	// Pending resource transitions are committed before a command list
// is executed on the command queue. This guarantees that resources will
// be in the expected state at the beginning of a command list.
#pragma endregion
	ResourceBarriers m_pendingResourceBarriers;

	// Resource barriers that need to be committed to the command list.
	ResourceBarriers m_resourceBarriers;

    using ResourceStateMap = std::unordered_map<ID3D12Resource*, ResourceState>;

	#pragma region Explanation...
	// The final (last known state) of the resources within a command list.
// The final resource state is committed to the global resource state when the 
// command list is closed but before it is executed on the command queue.
#pragma endregion
	ResourceStateMap m_finalResourceState;


	// The global resource state array (map) stores the state of a resource between command list execution.
	static ResourceStateMap ms_globalResourceState;

	// The mutex protects shared access to the GlobalResourceState map.
	static std::mutex ms_globalMutex;
	static bool ms_isLocked;

};

