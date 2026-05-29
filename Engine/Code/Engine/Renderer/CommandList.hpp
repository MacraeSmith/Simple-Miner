#pragma once
#include "Engine/Math/IntVec2.hpp"
#include <d3d12.h>

#include <map> 
#include <memory> 
#include <mutex> 
#include <vector> 
#include <cassert>

class CommandQueue;
class ResourceStateTracker;
class UploadBufferDX12;
class DynamicDescriptorHeap;
class RendererDX12;
class ResourceDX12;
class BufferDX12;
class ConstantBufferDX12;
class VertexBufferDX12;
class IndexBufferDX12;
class StructuredBufferDX12;
class RootSignatureDX12;
class TextureDX12;
enum class TextureUsage : int;
class Image;

struct TextureUploadInfo
{
	ID3D12Resource* m_uploadHeap = nullptr;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT m_footprint;
	uint64_t m_totalBytes;
	unsigned int m_numRows;
	uint64_t m_rowSizeInBytes;
};

class CommandList
{
public:
	CommandList(RendererDX12* renderer, CommandQueue* parentCommandQueue, D3D12_COMMAND_LIST_TYPE type, std::string const& name = "Added Command List");
	virtual ~CommandList();

	D3D12_COMMAND_LIST_TYPE		GetCommandListType() const { return m_d3d12CommandListType;}
	ID3D12GraphicsCommandList2* GetGraphicsCommandList() const {return m_d3d12CommandList;}
	DynamicDescriptorHeap*		GetDynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
	CommandQueue*				GetOwningCommandQueue() const {return m_owningCommandQueue;}
	void						SetName(std::string const& tempName);

	#pragma region Explanation...
	/**
	* Transition a resource to a particular state.
	*
	* @param resource The resource to transition.
	* @param stateAfter The state to transition the resource to. The before state is resolved by the resource state tracker.
	* @param subresource The subresource to transition. By default, this is D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES which indicates that all subresources are transitioned to the same state.
	* @param flushBarriers Force flush any barriers. Resource barriers need to be flushed before a command (draw, dispatch, or copy) that expects the resource to be in a particular state can run.
	*/
	#pragma endregion
	void		TransitionBarrier(ResourceDX12 const& resource, D3D12_RESOURCE_STATES stateAfter, unsigned int subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
	void		TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, unsigned int subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
	void		UAVBarrier(ResourceDX12 const& resource, bool flushBarriers = false);
	void		StageShaderResourceViewHandleOnly(uint32_t rootParameterIndex, uint32_t descriptorOffset, D3D12_CPU_DESCRIPTOR_HANDLE shaderResourceViewHandle);

	//Copy DX12 Objects
	//-------------------------------------------------------------------------------------------------------------------------------------------------
	void		CopyResource(ResourceDX12& dstRes, ResourceDX12 const& srcRes);
	void		CopyBuffer(BufferDX12& buffer, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	void		CopyVertexBuffer(VertexBufferDX12& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData);
	void		CopyIndexBuffer(IndexBufferDX12& indexBuffer, size_t numIndexes, DXGI_FORMAT indexFormat, const void* indexBufferData);
	void		CopyStructuredBuffer(StructuredBufferDX12& structuredBuffer, size_t numElements, size_t elementSize, const void* structuredBufferData, bool uavWriteAcess);
	void		UploadIntoExistingStructuredBuffer(StructuredBufferDX12& dst, size_t numElements, size_t elementSize, const void* srcData);
	void		FlushResourceBarriers();

	//Set DX12 Objects. Used for Binding Drawing and setting Constants
	//-------------------------------------------------------------------------------------------------------------------------------------------------
	void		SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
	void		SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
	void		SetComputeDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
	void		SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants); 
	void		SetVertexBuffer(uint32_t slot, VertexBufferDX12 const& vertexBuffer);
	void		SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData);
	void		SetIndexBuffer(IndexBufferDX12 const& indexBuffer);
	void		SetDynamicIndexBuffer(size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData);

	void		SetViewport(const D3D12_VIEWPORT& viewport);
	void		SetViewports(const std::vector<D3D12_VIEWPORT>& viewports);
	void		SetScissorRect(const D3D12_RECT& scissorRect);
	void		SetScissorRects(const std::vector<D3D12_RECT>& scissorRects);
	void		SetPipelineState(ID3D12PipelineState* pipelineState);
	void		SetGraphicsRootSignature(const RootSignatureDX12& rootSignature);
	void		SetComputeRootSignature(const RootSignatureDX12& rootSignature);
	void		SetShaderResourceView(uint32_t rootParameterIndex,uint32_t descriptorOffset, ResourceDX12 const& resource,D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					unsigned int firstSubresource = 0,unsigned int numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr);
	void		SetShaderResourceViewHandle(uint32_t rootParameterIndex, uint32_t descriptorOffset, ResourceDX12 const& resource, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					unsigned int firstSubresource = 0, unsigned int numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void		SetShaderResourceViewArray(uint32_t rootParameterIndex, uint32_t descriptorOffset, std::vector<ResourceDX12 const*> const& resources, D3D12_RESOURCE_STATES stateAfter, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr);
	void		SetUnorderedAccessView(uint32_t rootParameterIndex, uint32_t descriptorOffset, ResourceDX12 const& resource, D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS, unsigned int firstSubresource = 0, unsigned int numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_UNORDERED_ACCESS_VIEW_DESC const* uav = nullptr);
	void		SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);
	void		SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);
	//Draw
	//-------------------------------------------------------------------------------------------------------------------------------------------------
	void		Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0);
	void		DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);
	void		Dispatch(uint32_t numGroupsX, uint32_t numGroupsY = 1, uint32_t numGroupsZ = 1); //Compute Shader Dispatch

	//Object and Command List Management
	//-------------------------------------------------------------------------------------------------------------------------------------------------
	bool		Close(CommandList& pendingCommandList);
	void		Close();
	void		Reset();
	void		ReleaseTrackedObjects();

	//Textures
	//-------------------------------------------------------------------------------------------------------------------------------------------------
	void		LoadTextureFromFile(TextureDX12& texture, std::string const& filePathWithExtension);
	void		LoadTextureFromFileWithMips(TextureDX12& texture, std::string const& filePathWithExtension, int numMipLayers);
	void		LoadTextureFromImage(TextureDX12& texture, std::string const& imageName, Image const& image);
	void		LoadTextureFromImageWithMips(TextureDX12& texture, std::string const& imageName, Image const& image, int numMipLayers);

	void		BeginRendererEvent(char const* eventName) const;
	void		EndRendererEvent() const;

#pragma region Templates
	template<typename T>
	void CopyVertexBuffer(VertexBufferDX12& vertexBuffer, std::vector<T> const& vertexBufferData)
	{
		CopyVertexBuffer(vertexBuffer, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
	}
	template<typename T>
	void CopyIndexBuffer(IndexBufferDX12& indexBuffer, std::vector<T> const& indexBufferData)
	{
		assert(sizeof(T) == 2 || sizeof(T) == 4);

		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		CopyIndexBuffer(indexBuffer, indexBufferData.size(), indexFormat, indexBufferData.data());
	}

	template<typename T>
	void CopyStructuredBuffer(StructuredBufferDX12& structuredBuffer, std::vector<T> const& structuredBufferData, bool uavWriteAccess)
	{
		CopyStructuredBuffer(structuredBuffer, structuredBufferData.size(), sizeof(T), structuredBufferData.data(), uavWriteAccess);
	}

	/*
	template<typename T>
	void SetPersistantConstantBufferData(ConstantBufferDX12& constantBuffer, uint32_t rootParamaterIndex, T const& data)
	{
		SetPersistantConstantBufferData(constantBuffer, rootParamaterIndex, sizeof(T), &data);
	}
	*/

	template<typename T>
	void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, const T& data)
	{
		SetGraphicsDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
	}
	template<typename T>
	void SetGraphics32BitConstants(uint32_t rootParameterIndex, const T& constants)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
		SetGraphics32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
	}
	template<typename T>
	void SetComputeDynamicConstantBuffer(uint32_t rootParameterIndex, const T& data)
	{
		SetComputeDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
	}
	template<typename T>
	void SetCompute32BitConstants(uint32_t rootParameterIndex, const T& constants)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
		SetCompute32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
	}
	template<typename T>
	void SetDynamicVertexBuffer(uint32_t slot, const std::vector<T>& vertexBufferData)
	{
		SetDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
	}
	template<typename T>
	void SetDynamicIndexBuffer(const std::vector<T>& indexBufferData)
	{
		static_assert(sizeof(T) == 2 || sizeof(T) == 4);

		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		SetDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.data());
	}
#pragma endregion
private:
	void		TrackObject(ID3D12Object* object);
	void		TrackResource(ResourceDX12 const& res);
	void		TrackIntermediateResource(ResourceDX12 const& res);
	void		TrackIntermediateObject(ID3D12Object* object);

	void		BindDescriptorHeaps();
	TextureUploadInfo CreateTextureUploadBuffer(ID3D12Device* device, ID3D12Resource* textureResource);
	uint16_t	CalculateDestinationMipCount(IntVec2 const& dimensions, int requestedMipLevels);

private:
	RendererDX12*					m_renderer = nullptr;
	D3D12_COMMAND_LIST_TYPE				m_d3d12CommandListType;
	ID3D12GraphicsCommandList2*			m_d3d12CommandList = nullptr;
	ID3D12CommandAllocator*				m_d3d12CommandAllocator = nullptr;
	using TrackedObjects = std::vector <ID3D12Object*> ;
	TrackedObjects					m_trackedObjects;
	using IntermediateObjects = std::vector<ID3D12Object*>;
	IntermediateObjects				m_intermediateObjects;

	std::string m_name;


	std::unique_ptr<ResourceStateTracker> m_resourceStateTracker;
	std::unique_ptr<UploadBufferDX12>	m_uploadBuffer;
	std::unique_ptr<DynamicDescriptorHeap> m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	ID3D12DescriptorHeap*				m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
	ID3D12RootSignature*				m_rootSignature = nullptr;

	static std::map<std::string, ID3D12Resource*> ms_textureCache;
	static std::mutex ms_textureCacheMutex;

	CommandQueue* m_owningCommandQueue = nullptr;
};

