#pragma once
#include <d3d12.h>  // For ID3D12CommandQueue, ID3D12Device2, and ID3D12Fence
#include <string>

struct IDXGISwapChain4;

struct PipelineStateStream
{
	struct {
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
		ID3D12RootSignature* m_rootSignature = nullptr;
	} RootSignature;

	struct {
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
		D3D12_INPUT_LAYOUT_DESC m_inputLayoutDesc;
	} InputLayoutDesc;

	struct {
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE m_typologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	} PrimitiveTopologyType;

	struct {
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
		D3D12_SHADER_BYTECODE m_shaderByteCode;
	} VertexShader;

	struct {
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
		D3D12_SHADER_BYTECODE m_shaderByteCode;
	} PixelShader;

	struct {
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
		D3D12_DEPTH_STENCIL_DESC m_desc;
	} DepthStencilState;

	struct {
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
		DXGI_FORMAT m_format;
	} DepthStencilViewFormat;

	struct {
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
		D3D12_RT_FORMAT_ARRAY m_formats;
	} RenderTargetViewFormats;

	struct {
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
		D3D12_RASTERIZER_DESC m_desc = {};
	}RasterizerState;

	struct {
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
		D3D12_BLEND_DESC m_desc = {};
	}BlendState;


};

HRESULT D3DX12SerializeVersionedRootSignature(D3D12_VERSIONED_ROOT_SIGNATURE_DESC const* pRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION MaxVersion,
	_Outptr_ ID3DBlob** ppBlob, _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob);

UINT64					UpdateSubresources(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDestinationResource, ID3D12Resource* pIntermediate, UINT64 intermediateOffset,
	unsigned int firstSubresource, unsigned int numSubresources, D3D12_SUBRESOURCE_DATA* pSrcData);
UINT64					UpdateSubresources(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDestinationResource, ID3D12Resource* pIntermediate, _In_range_(0, D3D12_REQ_SUBRESOURCES) unsigned int FirstSubresource,
	_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) unsigned int numSubresources, UINT64 requiredSize, _In_reads_(numSubresources) D3D12_PLACED_SUBRESOURCE_FOOTPRINT const* pLayouts,
	_In_reads_(numSubresources) unsigned int const* pNumRows, _In_reads_(numSubresources) UINT64 const* pRowSizesInBytes, _In_reads_(numSubresources) D3D12_SUBRESOURCE_DATA const* pSrcData);
void					MemcpySubresource(D3D12_MEMCPY_DEST const* pDest, D3D12_SUBRESOURCE_DATA const* pSrc, SIZE_T rowSizeInBytes, unsigned int numRows, unsigned int numSlices);

void SetResourceName(ID3D12Resource* out_resource, std::string const& name);
void SetCommandQueueName(ID3D12CommandQueue* out_commandQueue, std::string const& name);
void SetCommandListName(ID3D12GraphicsCommandList* out_commandList, std::string const& name);
void SetCommandAllocatorName(ID3D12CommandAllocator* out_commandAllocator, std::string const& name);
void SetFenceName(ID3D12Fence* out_fence, std::string const& name);
void SetSwapChainName(IDXGISwapChain4* out_swapChain, std::string const& name);
void SetDescriptorHeapName(ID3D12DescriptorHeap* out_descriptorHeap, std::string const& name);
void SetPipelineStateName(ID3D12PipelineState* out_pipelineState, std::string const& name);
void SetRootSignatureName(ID3D12RootSignature* out_rootSignature, std::string const& name);

size_t AlignUp(size_t value, size_t alignment);

int	CalculateFullMipCount2D(int width, int height);


