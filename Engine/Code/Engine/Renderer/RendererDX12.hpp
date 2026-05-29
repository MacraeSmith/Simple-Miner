#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/CommandList.hpp"
#include "Engine/Renderer/DX12Utils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include <d3d12.h>  
#pragma comment(lib, "d3d12.lib")
#include <dxgi1_6.h>
#include <chrono>
#include <memory>
#undef OPAQUE
#ifdef ERROR
#undef ERROR
#endif // ERROR


class Window;
class CommandQueue;
class VertexBufferDX12;
class IndexBufferDX12;
class ShaderDX12;
class TextureDX12;
class RootSignatureDX12;
class DescriptorAllocator;
class DescriptorAllocation;
class ConstantBufferDX12;
class PipelineStateObjectDX12;
class StructuredBufferDX12;
class StructuredBufferViewDX12;
struct IDXGIAdapter4;
struct ID3D12Device2;

class CommandList;
typedef std::shared_ptr<CommandList> SharedCommandList;

constexpr int NUM_BUFFER_FRAMES = 3;

constexpr int NUM_CONSTANT_BUFFERS = 16;
enum class RootParameters : int
{
	PER_FRAME_CONSTANTS,		//rootParam 0 register (b1)
	CAMERA_CONSTANTS,			//rootParam 1 register (b2)
	MODEL_CONSTANTS,			//rootParam 2 register (b3)
	LIGHT_CONSTANTS,			//rootParam 3 register (b4)
	BINDLESS_TEXTURE_CONSTANTS,	//rootParam 4 register (b5)
	BINDLESS_STRUCTURED_BUFFER_CONSTANTS,	//rootParam 5 register(b6)
	CBV_6,						//rootParam 6 register(b7)
	CBV_7,						//rootParam 7 register (b8)
	TEXTURES = NUM_CONSTANT_BUFFERS,			  //rootParam 16 register (t0 - 4095)
	STRUCTURED_BUFFERS = NUM_CONSTANT_BUFFERS,    //Also root param 16
	TEXTURES_3D = NUM_CONSTANT_BUFFERS,			//also root param 16
	COUNT
};

enum class ComputeRootParameters : int
{
	CBV,
	CBV_1,
	CBV_2,
	CBV_3,
	CBV_4,
	SRV = NUM_CONSTANT_BUFFERS,
	UAV,
	COUNT
};

struct BindlessTextureConstants
{
	uint32_t m_textureIndexes[NUM_TEXTURE_SLOTS] = {};
};

constexpr int NUM_BUFFER_INDEXES = NUM_TEXTURE_SLOTS;
struct BindlessStructuredBufferConstants
{
	uint32_t m_bufferIndexes[NUM_BUFFER_INDEXES] = {};
};

static const int s_bindlessTextureConstantsSlot = 5;
constexpr int NUM_DEFAULT_TEXTURES = 3;
constexpr int MAX_NUM_BINDLESS_TEXTURES = 4096;
constexpr int MAX_NUM_BINDLESS_3D_TEXTURES = 4096;
constexpr int MAX_NUM_BINDLESS_STRUCTURED_BUFFERS = 4096;
constexpr int MAX_NUM_BINDLESS_SRVS = MAX_NUM_BINDLESS_TEXTURES + MAX_NUM_BINDLESS_STRUCTURED_BUFFERS + MAX_NUM_BINDLESS_3D_TEXTURES;
constexpr int STRUCTURED_BUFFER_OFFSET = MAX_NUM_BINDLESS_TEXTURES;

struct MipGenerationConstants
{
	uint32_t m_destinationMipWidth;
	uint32_t m_destinationMipHeight;
	uint32_t m_sourceMipWidth;
	uint32_t m_sourceMipHeight;
};

struct PendingMipGenerationRequest
{
	TextureDX12* m_texture = nullptr;
	IntVec2 m_baseDimensions;
	uint16_t m_mipCount = 1;
};


class RendererDX12 : public Renderer
{
public:
	RendererDX12(RendererConfig const& config);
	virtual ~RendererDX12() {};

	//Startup, Shutdown, and Frame Flow
	//--------------------------------------------------------------------------------
	virtual void	Startup() override;
	virtual void	BeginFrame() override;
	virtual void	EndFrame() override;
	unsigned int	Present();
	void			FlushGPU();
	virtual void	Shutdown() override;

	//Camera and Clear
	//--------------------------------------------------------------------------------
	virtual void	ClearScreen(Rgba8 const& clearColor) override;
	virtual void	ClearDepth() override;
	virtual void	BeginCamera(Camera const& camera) override;
	virtual void	EndCamera(Camera const& camera) override;

	virtual void	BeginRendererEvent(char const* eventName) const override;
	virtual void	EndRendererEvent() const override;

	//Binding
	//--------------------------------------------------------------------------------
	void			BindPSO(PipelineStateObjectDX12 const* pipelineStateObject);
	void			BindRootSignature(RootSignatureDX12* rootSignature);
	void			BindTexture(TextureDX12 const* texture, int slot = 0);
	void			BindShader(ShaderDX12 const* shader);
	void			BindStructuredBuffer(StructuredBufferDX12* buffer, int slot = 0);

	void			BindComputePSO(PipelineStateObjectDX12 const* pso);
	void			BindComputeRootSignature(RootSignatureDX12* rootSignature);
	void			BindComputeShader(ShaderDX12 const* shader);


	//Drawing
	//--------------------------------------------------------------------------------
	void			DrawVertexArray(Verts const& verts);
	void			DrawVertexArray(VertTBNs const& verts);
	void			DrawVertexBuffer(VertexBufferDX12* vbo, unsigned int vertexCount);
	void			DrawIndexedVertexBuffer(VertexBufferDX12* vbo, IndexBufferDX12* ibo, unsigned int indexCount);
	void			DrawInstancedVertexBuffer(VertexBufferDX12* vbo, unsigned int vertexCount, unsigned int instancedCount);
	void			DrawInstancedIndexedVertexBufer(VertexBufferDX12* vbo, IndexBufferDX12* ibo, unsigned int indexCount, unsigned int instanceCount);
	void			DrawFullScreenQuad(TextureDX12 const* colorTexture, TextureDX12 const* depthTexture, ShaderDX12 const* shader, AABB2 const& viewportBounds = AABB2::ZERO_TO_ONE);

	//Dispatch
	//--------------------------------------------------------------------------------
	void			Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE);

	//Creation
	//--------------------------------------------------------------------------------
	virtual BitmapFont* CreateOrGetBitMapFontFromFile(char const* bitmapFontFilePathWithNoExtension)override;
	TextureDX12*		CreateOrGetTextureFromFile(char const* imageFilePath, int numMipLevels = 0);
	TextureDX12*		CreateOrGetTextureFromImage(Image const& image, int numMipLevels = 0);
	TextureDX12*		CreateOrGetEmpty3DTexture(IntVec2 const& dimensions, int depth, std::string const& name);
	TextureDX12*		CreateEmpty3DTexture(IntVec2 const& dimensions, int depth, std::string const& name);
	void				QueueTextureForMipGeneration(TextureDX12* texture, IntVec2 const& dimensions, uint16_t destinationMipCount);

	bool				IsTextureLoaded(std::string const& filePath, TextureDX12*& out_texture, int numMipLevels = 0);
	VertexBufferDX12*	CreateVertexBuffer(Verts const& verts, std::string const& name = "New Vertex Buffer", CommandList* commandList = nullptr);
	VertexBufferDX12*	CreateVertexBuffer(VertTBNs const& verts, std::string const& name = "New Vertex Buffer", CommandList* commandList = nullptr);
	VertexBufferDX12*	CreateVertexBuffer(TerrainVerts const& verts, std::string const& name = "New Vertex Buffer", CommandList* commandList = nullptr);
	IndexBufferDX12*	CreateIndexBuffer(std::vector<unsigned int> indexes, std::string const& name = "New Index Buffer", CommandList* commandList = nullptr);

	ShaderDX12*			CreateOrGetShaderFromFile(char const* shaderName, VertexType vertexType = VertexType::VERTEX_PCU);
	ShaderDX12*			CreateOrGetComputeShaderFromFile(char const* shaderName);
	PipelineStateObjectDX12* CreateOrGetPSO(ShaderDX12 const* shader, RootSignatureDX12* rootSignature, BlendMode blendMode = BlendMode::OPAQUE, DepthMode depthMode = DepthMode::READ_WRITE_LESS_EQUAL,
		RasterizerMode rasterizerMode = RasterizerMode::SOLID_CULL_BACK, bool activeRenderTarget = true);


	StructuredBufferDX12* CreateEmptyStructuredBuffer(size_t numElements, size_t elementSize, std::string const& name, bool uavWriteAccess = true);
	StructuredBufferDX12* CreateStructuredBuffer(size_t numElements, size_t elementSize, const void* bufferData, bool writeAccess, std::string const& name);
	bool				  CanMakeStructuredBuffers(int numBuffersDesired) const;



	template<typename T>
	StructuredBufferDX12* CreateStructuredBuffer(std::vector<T> const& bufferData, bool writeAccess, std::string const& name)
	{
		return CreateStructuredBuffer(bufferData.size(), sizeof(T), bufferData.data(), writeAccess, name);
	}

	template<typename T>
	void UploadDataIntoStructuredBuffer(StructuredBufferDX12* structuredBuffer, std::vector<T> const& bufferData)
	{
		if ((structuredBuffer == nullptr) || (bufferData.empty()))
		{
			return;
		}

		m_directCommandList->UploadIntoExistingStructuredBuffer(*structuredBuffer, bufferData.size(), sizeof(T), bufferData.data());
	}

	void CopyDataIntoExistingStructuredBuffer(StructuredBufferDX12* buffer, size_t numElements, size_t elementSize, const void* bufferData);

	template<typename T>
	void CopyDataIntoExistingStructuredBuffer(StructuredBufferDX12* buffer, std::vector<T> const& bufferData)
	{
		CopyDataIntoExistingStructuredBuffer(buffer, bufferData.size(), sizeof(T), bufferData.data());
	}
	
	StructuredBufferViewDX12* GetStructuredBufferViewFromVertexBuffer(VertexBufferDX12 const* vbo);
	void				ClearStructuredBufferAtIndex(int index);

	//Render Target
	//--------------------------------------------------------------------------------
	RenderTarget		GetCopyOfCurrentRenderTarget();

	//Constant Buffers
	//--------------------------------------------------------------------------------
	template<typename T>
	void SetConstantBufferData(uint32_t rootParameterIndex, T const& data)
	{
		m_directCommandList->SetGraphicsDynamicConstantBuffer(rootParameterIndex, data);
	}

	template<typename T>
	void SetComputeConstantBufferData(uint32_t rootParameterIndex, T const& data, D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE)
	{
		SetComputeStatesIfChanged(commandListType);
		CommandList* commandList = GetCurrentCommandList(commandListType);
		commandList->SetComputeDynamicConstantBuffer(rootParameterIndex, data);
	}

	virtual void			SetModelConstants(Mat44 const& modelToWorldTransform = Mat44(), Rgba8 const& modelColor = Rgba8::WHITE) override;
	virtual void			SetLightConstants(Vec3 const& sunDirection, float sunIntensity, float ambientIntensity, Rgba8 const& sunColor = Rgba8::WHITE) override;
	virtual void			SetLightConstants(LightConstants const& lightConstants) override;
	virtual void			SetColorAdjustmentConstants(ColorAdjustmentConstants const& colorAdjustmentConstants) override;
	virtual void			SetPerFrameConstants(PerFrameConstants const& perFrameConstants) override;
	void					SetBindlessTextureConstants(BindlessTextureConstants const& textureConstants);
	void					SetBindlessStructuredBufferConstants(BindlessStructuredBufferConstants const& bufferConstants);


	void					BindGraphicsDescriptorHeaps();
	void					SetGraphicsSRV(uint32_t rootParameterIndex, uint32_t descriptorOffset, ResourceDX12 const& resource, D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, unsigned int firstSubresource = 0, unsigned int numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr);

	void					SetComputeSRV(uint32_t rootParameterIndex,uint32_t descriptorOffset, ResourceDX12* resource ,D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,unsigned int firstSubresource = 0,unsigned int numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr, D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE);
	void					SetComputeSRVFromView(uint32_t rootParameterIndex, uint32_t descriptorOffset, ResourceDX12* resource, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, unsigned int firstSubresource = 0, unsigned int numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void					SetComputeUAV(uint32_t rootParameterIndex,uint32_t descriptorOffset,ResourceDX12* resource,D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,unsigned int firstSubresource = 0,unsigned int numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_UNORDERED_ACCESS_VIEW_DESC const* uav = nullptr, D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE);
	void					TransitionResource(ResourceDX12& resource, D3D12_RESOURCE_STATES stateAfter, D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE);

	//Get DX12 Info
	//--------------------------------------------------------------------------------
	CommandQueue*						GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;
	CommandList*						GetCurrentCommandList(D3D12_COMMAND_LIST_TYPE type) const;
	ID3D12Device2*						GetDevice() const;
	unsigned int						GetCurrentBackBufferIndex() const;
	ID3D12Resource*						GetCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE			GetBackBufferRenderTargetView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE			GetCurrentDepthStencilView() const;
	DescriptorAllocator*				GetDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
	DescriptorAllocation				AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numDescriptors = 1) const;
	unsigned int						GetBindlessDescriptorIncrementSize() const { return m_bindlessDescriptorIncrementSize; }

	uint64_t							GetLastCompletedFenceValue(D3D12_COMMAND_LIST_TYPE type) const;

private:
	//Startup Initialization
	//--------------------------------------------------------------------------------
	ID3D12Device2*			CreateDevice(IDXGIAdapter4* adapter);
	IDXGIAdapter4*			GetAdapter(bool useWarp);
	bool					CheckTearingSupport();
	IDXGISwapChain4*		CreateSwapChain(HWND hWnd, ID3D12CommandQueue* commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount);
	ID3D12DescriptorHeap*	CreateDescriptorHeap(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
	ID3D12CommandAllocator* CreateCommandAllocator(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type);
	ID3D12Fence*			CreateFence(ID3D12Device2* device);
	HANDLE					CreateEventHandle();
	void					EnableDebugLayer();
	void					UpdateRenderTargetViews(ID3D12Device2* device, IDXGISwapChain4* swapChain, ID3D12DescriptorHeap* descriptorHeap);
	ID3D12DescriptorHeap*	CreateDescriptorHeapForDepthStencilView();
	void					CreateBindlessSRVHeap();
	D3D12_ROOT_PARAMETER1	CreateRootConstantBufferParameter(int shaderRegister, int registerSpace);
	RootSignatureDX12*		CreateDefaultBindlessTextureRootSignature();
	RootSignatureDX12*		CreateDefaultComputeRootSignature();

	//Shaders
	//--------------------------------------------------------------------------------
	ShaderDX12*				CreateShaderDX12(char const* shaderName, char const* shaderSource, VertexType vertexType = VertexType::VERTEX_PCU);
	ShaderDX12*				CreateShaderDX12(char const* shaderName, VertexType vertexType = VertexType::VERTEX_PCU);
	ShaderDX12*				CreateComputeShaderDX12(char const* shaderName, char const* shaderSource);
	ShaderDX12*				CreateComputeShaderDX12(char const* shaderName);

	//Textures / Render Targets
	//--------------------------------------------------------------------------------
	TextureDX12*			GetTextureForFileName(char const* imageFilePath, int numMipLevels = 0) const;
	TextureDX12*			CreateTextureFromImage(Image const& image);
	TextureDX12*			CreateTextureFromFile(char const* imageFilePath);
	TextureDX12*			CreateTextureFromFileWithMipMaps(char const* imageFilePath, int numMipLevels);
	TextureDX12*			CreateTextureFromImageWithMipMaps(Image const& image, int numMipLevels);
	uint32_t				ComputeMipDimension(uint32_t baseDimension, uint32_t mipIndex);
	void					GenerateMipsCompute(TextureDX12* texture, IntVec2 const& baseDimensions, uint16_t numMipLevels);


	RenderTarget			CreateRenderTarget(IntVec2 const& dimensions);
	TextureDX12*			CreateRenderTexture(IntVec2 const& dimensions);
	TextureDX12*			CreateDepthTexture(IntVec2 const& dimensions);
	RenderTarget			CreateCopyRenderTarget(IntVec2 const& dimensions);
	TextureDX12*			CreateCopyRenderTargetColor(IntVec2 const& dimensions);
	TextureDX12*			CreateCopyRenderTargetDepth(IntVec2 const& dimensions);

	//PSOs
	//--------------------------------------------------------------------------------
	PipelineStateObjectDX12* CreateGraphicsPSO(ShaderDX12 const* shader, RootSignatureDX12* rootSignature, BlendMode blendMode = BlendMode::OPAQUE, 
		DepthMode depthMode = DepthMode::READ_WRITE_LESS_EQUAL, RasterizerMode rasterizerMode = RasterizerMode::SOLID_CULL_BACK, bool activeRenderTarget = true);
	PipelineStateObjectDX12* CreateComputePSO(ShaderDX12 const* computeShader, RootSignatureDX12* rootSignature);
	std::string				GetGraphicsPSOName(ShaderDX12 const* shader, RootSignatureDX12* rootSignature, BlendMode blendMode, DepthMode depthMode, RasterizerMode rasterizerMode, bool activeRenderTarget) const;
	std::string				GetComputePSOName(ShaderDX12 const* computeShader, RootSignatureDX12* rootSignature) const;

	//Clearing
	//--------------------------------------------------------------------------------
	void					ClearRTV(D3D12_CPU_DESCRIPTOR_HANDLE rtv, float* clearColor) const;
	void					ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth = 1.0f) const;

	//Final Draw States
	//--------------------------------------------------------------------------------
	void					SetStatesIfChanged();
	void					SetComputeStatesIfChanged(D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE);
	void					StageDummyComputeSRV(uint32_t rootParameterIndex, uint32_t descriptorOffset, D3D12_COMMAND_LIST_TYPE commandListType);

	//Structured Buffer

	D3D12_CPU_DESCRIPTOR_HANDLE GetBindlessCpuHandle(uint32_t heapIndex) const;
	void CopySrvToBindlessHeap(uint32_t heapIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcCpuHandle);


public:

	RootSignatureDX12*			m_defaultTextureArrayRootSignature = nullptr;
private:
	bool m_vSync = true;
	bool m_tearingSupported = false;
	bool m_useWarp = false;

	//Shaders
	//--------------------------------------------------------------------------------
	ShaderDX12 const*					m_defaultShader = nullptr;
	std::vector<ShaderDX12*>			m_loadedShaders;
	ShaderDX12 const*					m_desiredShader = nullptr;
	ShaderDX12 const*					m_currentShader = nullptr;
	ShaderDX12 const*					m_defaultScreenCopyShader = nullptr;

	ShaderDX12 const*					m_desiredComputeShader = nullptr;
	ShaderDX12 const*					m_currentComputeShader = nullptr;
	ShaderDX12 const*					m_mipMapComputeShader = nullptr;


	//RootSignatures
	//--------------------------------------------------------------------------------
	std::vector<RootSignatureDX12*> m_loadedRootSignatures;
	RootSignatureDX12*			m_defaultRootSignature = nullptr;
	RootSignatureDX12*			m_desiredRootSignature = nullptr;
	RootSignatureDX12*			m_currentRootSignature = nullptr;

	RootSignatureDX12*			m_defaultComputeRootSignature = nullptr;
	RootSignatureDX12*			m_desiredComputeRootSignature = nullptr;
	RootSignatureDX12*			m_currentComputeRootSignature = nullptr;

	//Bindless Heap
	//--------------------------------------------------------------------------------
	bool m_rebindGraphicsDescriptorHeap = false;
	ID3D12DescriptorHeap* m_bindlessHeap = nullptr;
	unsigned int m_bindlessDescriptorIncrementSize = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE m_bindlessDescriptorHeapStartGPUHandle = {};


	//Textures
	//--------------------------------------------------------------------------------
	std::vector<TextureDX12*> m_loadedTextures;
	std::vector<TextureDX12*> m_loaded3DTextures;
	BindlessTextureConstants m_defaultTextureConstants;
	BindlessTextureConstants m_currentTextureConstants;
	bool m_uploadTextureConstants = true;

	std::vector<PendingMipGenerationRequest> m_pendingMipRequests;

	//Structured Buffers
	//--------------------------------------------------------------------------------
	std::vector<StructuredBufferDX12*> m_loadedStructuredBuffers;
	uint32_t m_defaultStructuredBufferIndex = 0;
	BindlessStructuredBufferConstants m_currentStructuredBufferConstants;
	bool m_uploadStructuredBufferConstants = true;

	//PSOs
	//--------------------------------------------------------------------------------
	std::vector<PipelineStateObjectDX12*> m_loadedPipelineStateObjects;
	PipelineStateObjectDX12 const* m_desiredPSO = nullptr;
	PipelineStateObjectDX12 const* m_currentPSO = nullptr;

	PipelineStateObjectDX12 const*	m_desiredComputePSO = nullptr;
	PipelineStateObjectDX12 const*	m_currentComputePSO = nullptr;

	//PSO states
	//--------------------------------------------------------------------------------
	BlendMode		m_currentBlendMode = BlendMode::ALPHA;
	SamplerMode		m_currentSamplerModeBySlot[NUM_TEXTURE_SLOTS] = {};
	RasterizerMode	m_currentRasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	DepthMode		m_currentDepthMode = DepthMode::READ_WRITE_LESS_EQUAL;

	//CommandQueues
	//--------------------------------------------------------------------------------
	CommandQueue* m_directCommandQueue = nullptr;
	CommandQueue* m_computeCommandQueue = nullptr;
	CommandQueue* m_copyCommandQueue = nullptr;

	//Active Command Lists
	//--------------------------------------------------------------------------------
	CommandList* m_directCommandList = nullptr;
	CommandList* m_copyCommandList = nullptr;
	CommandList* m_computeCommandList = nullptr;

	uint64_t	m_directCommandFence;
	uint64_t	m_copyCommandFence;
	uint64_t	m_computeCommandFence;

	//Compute
	//--------------------------------------------------------------------------------
	D3D12_COMMAND_LIST_TYPE m_lastComputeCommandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE;

	D3D12_CPU_DESCRIPTOR_HANDLE m_dummySrvHandle;
	bool m_computeSrvWasBoundThisDispatch = false;

	//Misc DX12 Objects
	//--------------------------------------------------------------------------------
	ID3D12Device2*			m_device = nullptr;
	IDXGISwapChain4*		m_swapChain = nullptr;
	ID3D12Resource*			m_backBuffers[NUM_BUFFER_FRAMES] = {};
	ID3D12DescriptorHeap*	m_offscreenRTVDescriptorHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_offscreenRTVDescriptorHeapStartHandle = {};
	unsigned int			m_offscreenRTVDescriptorSize = 0;
	unsigned int			m_currentBackBufferIndex = 0;
	uint64_t				m_frameFenceValues[NUM_BUFFER_FRAMES] = {};
	mutable DescriptorAllocator*	m_descriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

};

