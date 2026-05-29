#include "Engine/Renderer/ResourceDX12.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/CommandQueue.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/VertexBufferDX12.hpp"
#include "Engine/Renderer/StructuredBufferDX12.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/ShaderDX12.hpp"
#include "Engine/Renderer/RootSignatureDX12.hpp"
#include "Engine/Renderer/PipelineStateObjectDX12.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Renderer/DefaultShader.hpp"
#include "Engine/Renderer/IndexBufferDX12.hpp"
#include "Engine/Renderer/CommandList.hpp"
#include "Engine/Renderer/TextureDX12.hpp"
#include "Engine/Renderer/DynamicDescriptorHeap.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/ImGui/ImGuiSystem.hpp"
#include "Engine/Core/StaticMesh.hpp"
#include "Engine/Renderer/ResourceStateTracker.hpp"
#include "Engine/Core/Time.hpp"

#include <cassert>

#include <d3dcompiler.h>
#include <dxgi.h>
//#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dCompiler.lib")

#pragma comment(lib, "dxguid.lib")
#include <dxgidebug.h>
#include <DirectXMath.h>



extern Window* g_window;

RendererDX12::RendererDX12(RendererConfig const& config)
	:Renderer(config)
{
}
//Startup, Shutdown, and Frame Flow
//--------------------------------------------------------------------------------

void RendererDX12::Startup()
{
	if (!DirectX::XMVerifyCPUSupport())
	{
		ERROR_AND_DIE("Failed to verify DirectX math library support");
		return;
	}

#if defined(ENGINE_DEBUG_RENDERER)

	EnableDebugLayer();

#endif
	//Check if tearing is supported 
	m_tearingSupported = CheckTearingSupport();

	//Create Device
	//--------------------------------------------------------------------------------
	IDXGIAdapter4* dxgiAdapter4 = GetAdapter(m_useWarp);
	dxgiAdapter4->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("dxgiAdapter4") + 1, "dxgiAdapter4");
	m_device = CreateDevice(dxgiAdapter4);
	m_device->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("m_device") + 1, "m_device");

	
	//Create Command Queues
	//--------------------------------------------------------------------------------
	m_directCommandQueue = new CommandQueue(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_computeCommandQueue = new CommandQueue(this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_copyCommandQueue = new CommandQueue(this, D3D12_COMMAND_LIST_TYPE_COPY);

	m_directCommandList = m_directCommandQueue->GetCommandList();
	m_copyCommandList = m_copyCommandQueue->GetCommandList();
	m_computeCommandList = m_computeCommandQueue->GetCommandList();
	
	
	//Create Command Queues
	//--------------------------------------------------------------------------------
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_descriptorAllocators[i] = new DescriptorAllocator((D3D12_DESCRIPTOR_HEAP_TYPE) i, this);
	}

	//Create Swap Chain
	//--------------------------------------------------------------------------------
	IntVec2 clientDims = g_window->GetClientDimensions();
	m_swapChain = CreateSwapChain((HWND)g_window->GetHwnd(), m_directCommandQueue->GetD3D12CommandQueue(),
		(uint32_t)clientDims.x, (uint32_t)clientDims.y, NUM_BUFFER_FRAMES);

	SetSwapChainName(m_swapChain, "m_swapChain");

	//Set current back buffer index
	//--------------------------------------------------------------------------------
	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	//Create rtvDescriptor heap and size
	//--------------------------------------------------------------------------------
	m_offscreenRTVDescriptorHeap = CreateDescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_BUFFER_FRAMES);
	m_offscreenRTVDescriptorHeapStartHandle = m_offscreenRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_offscreenRTVDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	SetDescriptorHeapName(m_offscreenRTVDescriptorHeap, "m_offScreenRTVDescriptorHeap");

	//Update rtv
	//--------------------------------------------------------------------------------
	UpdateRenderTargetViews(m_device, m_swapChain, m_offscreenRTVDescriptorHeap);

	for (int i = 0; i < NUM_BUFFER_FRAMES; ++i)
	{
		ID3D12Resource* backBuffer = m_backBuffers[i];
		ResourceStateTracker::AddGlobalResourceState(backBuffer, D3D12_RESOURCE_STATE_PRESENT);
	}

	//Create Default Shaders and RootSig
	//--------------------------------------------------------------------------------
	DefaultShader shaderDefault;
	m_defaultShader = CreateShaderDX12("Default", shaderDefault.m_defaultShaderDX12Source);
	m_mipMapComputeShader = CreateComputeShaderDX12("MipMap_Compute", shaderDefault.m_defaultMipComputeShaderDX12);
	
	m_defaultRootSignature = CreateDefaultBindlessTextureRootSignature();
	BindRootSignature(m_defaultRootSignature);

	m_defaultComputeRootSignature = CreateDefaultComputeRootSignature();




	//Init Textures
	//--------------------------------------------------------------------------------
	DX_SAFE_RELEASE(dxgiAdapter4);

	CreateBindlessSRVHeap();
	
	Image whitePixelImage(IntVec2(2, 2), Rgba8::WHITE, "White");
	TextureDX12* whiteTexture = CreateTextureFromImage(whitePixelImage);
	unsigned int whiteTextureIndex = whiteTexture->GetBindlessIndex();
	m_defaultTextureConstants.m_textureIndexes[0] = whiteTextureIndex;

	Image normalMapImage(IntVec2(2, 2), Rgba8::DEFAULT_NORMAL_MAP, "Normal");
	TextureDX12* normalMapTexture = CreateTextureFromImage(normalMapImage);
	m_defaultTextureConstants.m_textureIndexes[1] = normalMapTexture->GetBindlessIndex();

	Image specGlossEmitImage(IntVec2(2, 2), Rgba8::DEFAULT_SPEC_GLOSS_EMIT_MAP, "SpecGlossEmit");
	TextureDX12* specGlossEmitTexture = CreateTextureFromImage(specGlossEmitImage);
	m_defaultTextureConstants.m_textureIndexes[2] = specGlossEmitTexture->GetBindlessIndex();

	for (int i = 3; i < NUM_TEXTURE_SLOTS; ++i)
	{
		m_defaultTextureConstants.m_textureIndexes[i] = whiteTextureIndex;
	}

	m_currentTextureConstants = m_defaultTextureConstants;

	Image blackImage(IntVec2(2, 2), Rgba8::BLACK, "Black");
	Image greyImage(IntVec2(2, 2), Rgba8::GREY, "Grey");
	CreateTextureFromImage(blackImage);
	CreateTextureFromImage(greyImage);

	//Dummy SRV handle for compute dispatches that do not use an SRV
	Image dummySRVImage(IntVec2(2,2), Rgba8::MAGENTA, "DummyComputeSRVTexture");
	TextureDX12* dummySRVTexture = CreateTextureFromImage(dummySRVImage);
	m_dummySrvHandle = dummySRVTexture->GetShaderResourceView(nullptr);

#ifdef RENDERER_DX12

	//Render targets
	//--------------------------------------------------------------------------------
	m_renderTarget = CreateRenderTarget(clientDims);
	m_directCommandList->TransitionBarrier(*m_renderTarget.m_color, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_directCommandList->TransitionBarrier(*m_renderTarget.m_depth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_directCommandList->TransitionBarrier(*m_renderTarget.m_mask, D3D12_RESOURCE_STATE_RENDER_TARGET);

	for (int i = 0; i < NUM_COPY_RENDER_TARGETS; ++i)
	{
		m_renderTargetCopies[i] = CreateCopyRenderTarget(clientDims);
	}

	m_defaultScreenCopyShader = CreateShaderDX12("PostProcess", shaderDefault.m_defaultShaderDX12FullScreenCopySource);

	//Init ImGui
	//--------------------------------------------------------------------------------
	if (m_config.m_useImGUI)
	{
		ImGuiConfig config;
		config.m_renderer = this;
		config.m_window = Window::s_mainWindow;
		config.m_darkMode = true;
		g_imGuiSystem = new ImGuiSystem(config);
		g_imGuiSystem->Startup();
	}
#endif // RENDERER_DX12
}

void RendererDX12::BeginFrame()
{
#ifdef RENDERER_DX12

	//Execute any open command lists
	//-----------------------------------------------------------------------------------
	if (m_copyCommandList)
	{
		m_copyCommandFence = m_copyCommandQueue->ExecuteCommandList(m_copyCommandList);
	}

	if (m_computeCommandList)
	{
		m_computeCommandQueue->Wait(*m_copyCommandQueue);
		m_copyCommandFence = m_computeCommandQueue->ExecuteCommandList(m_computeCommandList);
	}

	if (m_directCommandList)
	{
		m_directCommandQueue->Wait(*m_directCommandQueue);
		m_directCommandFence = m_directCommandQueue->ExecuteCommandList(m_directCommandList);
	}


	//Get command lists for this frame
	//-----------------------------------------------------------------------------------
	m_directCommandList = m_directCommandQueue->GetCommandList();
	m_directCommandList->SetName("Current Renderer Direct CmdList");

	m_copyCommandList = m_copyCommandQueue->GetCommandList();
	m_copyCommandList->SetName("Current Renderer Copy CmdList");

	m_computeCommandList = m_computeCommandQueue->GetCommandList();
	m_computeCommandList->SetName("Current Renderer Compute CmdList");
	m_currentComputePSO = nullptr;
	m_currentComputeRootSignature = nullptr;

	// Clear Render Targets for this frame
	//-----------------------------------------------------------------------------------

	m_activeRenderTarget = true;
	ID3D12GraphicsCommandList* d3d12CommandList = m_directCommandList->GetGraphicsCommandList();
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_renderTarget.m_color->GetRenderTargetView();
	D3D12_CPU_DESCRIPTOR_HANDLE maskRtv = m_renderTarget.m_mask->GetRenderTargetView();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_renderTarget.m_depth->GetDepthStencilView();
	ClearScreen(Rgba8::ORANGE);
	ClearDepthStencilView(dsv);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2] = {};
	rtvs[0] = rtv;
	rtvs[1] = maskRtv;
	d3d12CommandList->OMSetRenderTargets(2, rtvs, false, &dsv);

	m_directCommandList->SetGraphicsRootSignature(*m_defaultRootSignature); //#TODO if I add more root signatures this location will have to change
	m_directCommandList->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //This probably will never change
	BindGraphicsDescriptorHeaps();

	//Initialize ImGUI
	//-----------------------------------------------------------------------------------
	if (g_imGuiSystem)
	{
		g_imGuiSystem->BeginFrame();
	}

#endif // RENDERER_DX12

}

void RendererDX12::EndFrame()
{
#ifdef RENDERER_DX12
	//Execute copy and compute command list
	//-----------------------------------------------------------------------------------
	double startTime = GetCurrentTimeSeconds();
	m_copyCommandFence = m_copyCommandQueue->ExecuteCommandList(m_copyCommandList);

	m_computeCommandQueue->Wait(*m_copyCommandQueue);
	for (int i = 0; i < (int)m_pendingMipRequests.size(); ++i)
	{
		PendingMipGenerationRequest const& mipRequest = m_pendingMipRequests[i];
		GenerateMipsCompute(mipRequest.m_texture, mipRequest.m_baseDimensions, mipRequest.m_mipCount);
		m_directCommandList->TransitionBarrier(*mipRequest.m_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
		m_directCommandQueue->QueueResourceForMarkedUse(mipRequest.m_texture);
	}

	m_pendingMipRequests.clear();
	m_computeCommandFence = m_computeCommandQueue->ExecuteCommandList(m_computeCommandList);

	//m_directCommandQueue->Wait(*m_copyCommandQueue);
	m_directCommandQueue->Wait(*m_computeCommandQueue);
	m_copyCommandList = nullptr;
	m_computeCommandList = nullptr;


	//Get Render Target from draw calls this frame
	//-----------------------------------------------------------------------------------
	m_activeRenderTarget = false;
	RenderTarget copyRT = GetCopyOfCurrentRenderTarget();

	//Transition back buffer for drawing and draw directly to back buffer
	//-----------------------------------------------------------------------------------
	ID3D12Resource* backBuffer = GetCurrentBackBuffer();
	m_directCommandList->TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_directCommandList->FlushResourceBarriers();

	D3D12_CPU_DESCRIPTOR_HANDLE bbRtv = GetBackBufferRenderTargetView();
	ID3D12GraphicsCommandList* d3d12CommandList = m_directCommandList->GetGraphicsCommandList();
	d3d12CommandList->OMSetRenderTargets(1, &bbRtv, false, nullptr); // back buffer has no depth

	//Draws (full screen quad + ImGui)
	DrawFullScreenQuad(copyRT.m_color, copyRT.m_depth, m_defaultScreenCopyShader);
	if (g_imGuiSystem)
	{
		g_imGuiSystem->EndFrame();
	}

	//Transition Back buffer to present and execute command list
	//-----------------------------------------------------------------------------------
	m_directCommandList->TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT);
	m_directCommandFence = m_directCommandQueue->ExecuteCommandList(m_directCommandList);
	m_directCommandList = nullptr;

	double duration = GetCurrentTimeSeconds() - startTime;
	EventArgs args;
	args.SetValue("Duration", Stringf("%f", duration));
	FireEvent("RenderDuration", args);

	Present();

#endif // RENDERER_DX12
}

unsigned int RendererDX12::Present()
{
	unsigned int syncInterval = m_vSync ? 1 : 0;
	unsigned int presentFlags = m_tearingSupported && !m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	HRESULT hr = m_swapChain->Present(syncInterval, presentFlags);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), Stringf("Could not present from swap chain. Error: 0x%08x", hr));
	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	return m_currentBackBufferIndex;
}

void RendererDX12::FlushGPU()
{
	m_directCommandQueue->Flush();
	m_computeCommandQueue->Flush();
	m_copyCommandQueue->Flush();
}

void RendererDX12::Shutdown()
{

	if (g_imGuiSystem)
	{
		g_imGuiSystem->Shutdown();
		delete g_imGuiSystem;
		g_imGuiSystem = nullptr;
	}
	
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		delete(m_descriptorAllocators[i]);
		m_descriptorAllocators[i] = nullptr;
	}

	for (int i = 0; i < NUM_BUFFER_FRAMES; ++i)
	{
		DX_SAFE_RELEASE(m_backBuffers[i]);
	}

	for (int i = 0; i < (int)m_loadedPipelineStateObjects.size(); ++i)
	{
		delete(m_loadedPipelineStateObjects[i]);
		m_loadedPipelineStateObjects[i] = nullptr;
	}

	for (int i = 0; i < (int)m_loadedShaders.size(); ++i)
	{
		delete(m_loadedShaders[i]);
		m_loadedShaders[i] = nullptr;
	}

	for (int i = 0; i < (int)m_loadedTextures.size(); ++i)
	{
		delete(m_loadedTextures[i]);
		m_loadedTextures[i] = nullptr;
	}

	for (int i = 0; i < (int)m_loaded3DTextures.size(); ++i)
	{
		delete(m_loaded3DTextures[i]);
		m_loaded3DTextures[i] = nullptr;
	}

	for (int i = 0; i < (int)m_loadedStructuredBuffers.size(); ++i)
	{
		delete(m_loadedStructuredBuffers[i]);
		m_loadedStructuredBuffers[i] = nullptr;
	}

#ifdef RENDERER_DX12
	delete(m_renderTarget.m_color);
	m_renderTarget.m_color = nullptr;
	delete(m_renderTarget.m_depth);
	m_renderTarget.m_depth = nullptr;
	delete(m_renderTarget.m_mask);
	m_renderTarget.m_mask = nullptr;
#endif // RENDERER_DX12

	for(int i = 0; i < (int)m_loadedStaticMeshes.size(); ++i)
	{
		delete m_loadedStaticMeshes[i];
		m_loadedStaticMeshes[i] = nullptr;
	}

	for (int i = 0; i < (int)m_loadedRootSignatures.size(); ++i)
	{
		delete m_loadedRootSignatures[i];
		m_loadedRootSignatures[i] = nullptr;
	}

	DX_SAFE_RELEASE(m_bindlessHeap);
	DX_SAFE_RELEASE(m_offscreenRTVDescriptorHeap);

	//delete m_directCommandList;
	//delete m_copyCommandList;
	//delete m_computeCommandList;

	m_directCommandList = nullptr;
	m_copyCommandList = nullptr;
	m_computeCommandList = nullptr;

	if (g_window->IsFullScreen())
	{
		m_swapChain->SetFullscreenState(FALSE, nullptr);
	}

	DX_SAFE_RELEASE(m_swapChain);

	
	delete m_copyCommandQueue;
	m_copyCommandQueue = nullptr;
	delete m_computeCommandQueue;
	m_computeCommandQueue = nullptr;
	delete m_directCommandQueue;
	m_directCommandQueue = nullptr;


	
	DX_SAFE_RELEASE(m_device);
#if defined(ENGINE_DEBUG_RENDERER)
	IDXGIDebug1* dxgiDebug =nullptr;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	dxgiDebug->Release();
	dxgiDebug = nullptr;
#endif
	/*
	*/

	/*
#if defined(ENGINE_DEBUG_RENDERER)
	ID3D12DebugDevice* debugDevice = nullptr;
	if (m_device && SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&debugDevice))))
	{
		debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		debugDevice->Release();
		debugDevice = nullptr;
	}
#endif

	DX_SAFE_RELEASE(m_device);
	*/

}


//Startup Initialization
//-----------------------------------------------------------------------------------
IDXGIAdapter4* RendererDX12::GetAdapter(bool useWarp)
{
	IDXGIFactory4* dxgiFactory;
	UINT createFactoryFlags = 0;
	HRESULT hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not create D3D12 factory flags");

	IDXGIAdapter1* dxgiAdapter1 = nullptr;
	IDXGIAdapter4* dxgiAdapter4 = nullptr;
	if (useWarp)
	{
		hr = dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1));
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create warp adapters");
		hr = dxgiAdapter1->QueryInterface(IID_PPV_ARGS(&dxgiAdapter4));
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not query COM object type");
	}

	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1,
					D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				hr = dxgiAdapter1->QueryInterface(IID_PPV_ARGS(&dxgiAdapter4));
				GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not query COM object type");
			}
		}
	}

	DX_SAFE_RELEASE(dxgiAdapter1);
	DX_SAFE_RELEASE(dxgiFactory);

	return dxgiAdapter4;
}

ID3D12Device2* RendererDX12::CreateDevice(IDXGIAdapter4* adapter)
{
	//#TODO: Change DX12 device to Device10?
	ID3D12Device2* d3d12Device2 = nullptr;

	//TODO: Change DX12 device to D3D_FEATURE_LEVEL_12_2
	HRESULT hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Create D3D12 Device failed");

	//#DX12_RELEASE
	DX_SAFE_RELEASE(adapter);

	// Enable debug messages in debug mode.


#if defined(ENGINE_DEBUG_RENDERER)
	ID3D12InfoQueue* pInfoQueue;
	if (SUCCEEDED(d3d12Device2->QueryInterface(IID_PPV_ARGS(&pInfoQueue))))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		//pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress whole categories of messages
 //D3D12_MESSAGE_CATEGORY Categories[] = {};

 // Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		hr = pInfoQueue->PushStorageFilter(&NewFilter);
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not push storage filter");

		DX_SAFE_RELEASE(pInfoQueue);
	}
#endif

	return d3d12Device2;
}

bool RendererDX12::CheckTearingSupport()
{
	bool allowTearing = false;
	IDXGIFactory4* factory4 = nullptr;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		IDXGIFactory5* factory5 = nullptr;
		if (SUCCEEDED(factory4->QueryInterface(IID_PPV_ARGS(&factory5))))
		{
			if (FAILED(factory5->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing))))
			{
				allowTearing = false;
			}
		}
		factory5->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("factory5") + 1, "factory5");
	}

	factory4->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("factory4_2") + 1, "factory4_2");


	return allowTearing == true;
}

IDXGISwapChain4* RendererDX12::CreateSwapChain(HWND hWnd, ID3D12CommandQueue* commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount)
{
	IDXGISwapChain4* dxgiSwapChain4 = nullptr;
	IDXGIFactory4* dxgiFactory4 = nullptr;
	UINT createFactoryFlags = 0;

	HRESULT hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create factory flags");

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	IDXGISwapChain1* swapChain1 = nullptr;
	hr = dxgiFactory4->CreateSwapChainForHwnd(
		commandQueue,
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1);

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create swap chaing for Hwnd");

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	hr = dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not make windows assosiaction")

		hr = swapChain1->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain4));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not query dxgiSwapChain4");

	//#DX12_RELEASE
	DX_SAFE_RELEASE(swapChain1);

	return dxgiSwapChain4;
}

ID3D12DescriptorHeap* RendererDX12::CreateDescriptorHeap(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;

	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not create descriptor heap");

	return descriptorHeap;
}

void RendererDX12::UpdateRenderTargetViews(ID3D12Device2* device, IDXGISwapChain4* swapChain, ID3D12DescriptorHeap* descriptorHeap)
{
	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < NUM_BUFFER_FRAMES; ++i)
	{
		ID3D12Resource* backBuffer = nullptr;
		HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not get swap chain buffer");

		device->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);

		m_backBuffers[i] = backBuffer;
		std::string bufferName = Stringf("m_backBuffer_%i", i);
		m_backBuffers[i]->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(bufferName.c_str()) + 1, bufferName.c_str());


		//rtvHandle.Offset(rtvDescriptorSize);
		rtvHandle.ptr += rtvDescriptorSize;
	}
}

ID3D12CommandAllocator* RendererDX12::CreateCommandAllocator(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type)
{
	ID3D12CommandAllocator* commandAllocator = nullptr;
	HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create command allocator");

	std::string typeText;
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		typeText = "Direct";
		break;
	case D3D12_COMMAND_LIST_TYPE_BUNDLE:
		typeText = "Bundle";
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		typeText = "Compute";
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		typeText = "Copy";
		break;
	case D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE:
		typeText = "VideoDecode";
		break;
	case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS:
		typeText = "VideoProcess";
		break;
	case D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE:
		typeText = "VideoEncode";
		break;
	default:
		break;
	}

	std::string allocatorName = Stringf("commandAllocator_%s", typeText.c_str());
	SetCommandAllocatorName(commandAllocator, allocatorName);
	return commandAllocator;
}

ID3D12Fence* RendererDX12::CreateFence(ID3D12Device2* device)
{
	ID3D12Fence* fence = nullptr;
	HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create Fence");

	return fence;
}

HANDLE RendererDX12::CreateEventHandle()
{
	HANDLE fenceEvent;

	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create fence event.");

	return fenceEvent;
}

void RendererDX12::EnableDebugLayer()
{
	ID3D12Debug1* debugInterface = nullptr;
	HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not get debug interface");
	debugInterface->EnableDebugLayer();
	//debugInterface->SetEnableGPUBasedValidation(TRUE);
}

DescriptorAllocation RendererDX12::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numDescriptors) const
{
	return m_descriptorAllocators[type]->Allocate(numDescriptors);
}

ID3D12DescriptorHeap* RendererDX12::CreateDescriptorHeapForDepthStencilView()
{
	ID3D12DescriptorHeap* newDSV = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&newDSV));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not create Descriptor Heap");
	return newDSV;
}

D3D12_CPU_DESCRIPTOR_HANDLE RendererDX12::GetBindlessCpuHandle(uint32_t heapIndex) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE h = m_bindlessHeap->GetCPUDescriptorHandleForHeapStart();
	h.ptr = (UINT64)(h.ptr + ((UINT64)heapIndex * (UINT64)m_bindlessDescriptorIncrementSize));
	return h;
}

void RendererDX12::CopySrvToBindlessHeap(uint32_t heapIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcCpuHandle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE dstCpuHandle = GetBindlessCpuHandle(heapIndex);

	// Copy 1 SRV descriptor from your CPU-only heap into the shader-visible heap
	m_device->CopyDescriptorsSimple(
		1,
		dstCpuHandle,
		srcCpuHandle,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);
}

//Get Render Objects Helpers
//-----------------------------------------------------------------------------------

ID3D12Device2* RendererDX12::GetDevice() const
{
	return m_device;
}

unsigned int RendererDX12::GetCurrentBackBufferIndex() const
{
	return m_currentBackBufferIndex;
}

ID3D12Resource* RendererDX12::GetCurrentBackBuffer() const
{
	return m_backBuffers[m_currentBackBufferIndex];
}

D3D12_CPU_DESCRIPTOR_HANDLE RendererDX12::GetBackBufferRenderTargetView() const
{
#ifdef RENDERER_DX12

	if (m_activeRenderTarget)
	{
		ERROR_AND_DIE("Getting Back buffer RTV when still using Render Texture as Target");
	}

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_offscreenRTVDescriptorHeapStartHandle;
	handle.ptr += m_currentBackBufferIndex * m_offscreenRTVDescriptorSize;
	return handle;
#else
	return D3D12_CPU_DESCRIPTOR_HANDLE {};
#endif // RENDERER_DX12
}

D3D12_CPU_DESCRIPTOR_HANDLE RendererDX12::GetCurrentDepthStencilView() const
{
#ifdef RENDERER_DX12

	if (m_activeRenderTarget)
	{
		return m_renderTarget.m_depth->GetDepthStencilView();
	}

	else
		ERROR_AND_DIE("Attempting to get Depth Stencil View without active render target");
#else
	ERROR_AND_DIE("Attempting to get Depth Stencil View without active render target");
	//D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
	//return handle;
#endif // RENDERER_DX12
}

DescriptorAllocator* RendererDX12::GetDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	return m_descriptorAllocators[(int)type];
}

CommandQueue* RendererDX12::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{

	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT: return m_directCommandQueue;

	case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_computeCommandQueue;

	case D3D12_COMMAND_LIST_TYPE_COPY: return m_copyCommandQueue;

	}

	return nullptr;
}

CommandList* RendererDX12::GetCurrentCommandList(D3D12_COMMAND_LIST_TYPE type) const
{
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT: return m_directCommandList;

	case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_computeCommandList;

	case D3D12_COMMAND_LIST_TYPE_COPY: return m_copyCommandList;

	}

	return nullptr;
}

//Textures
//-----------------------------------------------------------------------------------

//retrieval
TextureDX12* RendererDX12::CreateOrGetTextureFromFile(char const* imageFilePath, int numMipLevels)
{
	TextureDX12* foundTexture = GetTextureForFileName(imageFilePath, numMipLevels);
	if (foundTexture)
	{
		return foundTexture;
	}

	if (numMipLevels > 0)
	{
		return CreateTextureFromFileWithMipMaps(imageFilePath, numMipLevels);
	}

	return CreateTextureFromFile(imageFilePath);
}

TextureDX12* RendererDX12::CreateOrGetTextureFromImage(Image const& image, int numMipLevels)
{
	std::string imageName = GetFileName(image.GetImageFilePath());
	TextureDX12* foundTexture = GetTextureForFileName(imageName.c_str(), numMipLevels);
	if (foundTexture)
	{
		return foundTexture;
	}

	if (numMipLevels > 0)
	{
		return CreateTextureFromImageWithMipMaps(image, numMipLevels);
	}


	return CreateTextureFromImage(image);
}

TextureDX12* RendererDX12::CreateOrGetEmpty3DTexture(IntVec2 const& dimensions, int depth, std::string const& name)
{
	for (int i = 0; i < (int)m_loaded3DTextures.size(); ++i)
	{
		std::string textureName = m_loadedTextures[i]->GetName();
		if (textureName == name)
		{
			if (m_loaded3DTextures[i]->GetDimensions() == dimensions && m_loaded3DTextures[i]->GetDepth() == (uint32_t)depth)
			{
				return m_loaded3DTextures[i];
			}
		}
	}
	return CreateEmpty3DTexture(dimensions, depth, name);
}

TextureDX12* RendererDX12::CreateEmpty3DTexture(IntVec2 const& dimensions, int depth, std::string const& name)
{
	if ((int)m_loaded3DTextures.size() >= MAX_NUM_BINDLESS_3D_TEXTURES)
	{
		ERROR_AND_DIE(Stringf("Number of bindless 3D textures exceeded maximum: %i", MAX_NUM_BINDLESS_3D_TEXTURES));
	}

	TextureDX12* newTexture = new TextureDX12(this, name, nullptr);
	newTexture->SetDimensions(dimensions);
	newTexture->SetDepth((uint32_t)depth);

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	textureDesc.Alignment = 0;
	textureDesc.Width = (UINT64)dimensions.x;
	textureDesc.Height = (UINT)dimensions.y;
	textureDesc.DepthOrArraySize = (UINT16)depth;
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	ID3D12Resource* textureResource = nullptr;
	HRESULT hr = m_device->CreateCommittedResource(&heapProps,D3D12_HEAP_FLAG_NONE,&textureDesc,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&textureResource));

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create empty 3D texture");

	newTexture->SetD3D12Resource(textureResource);
	newTexture->CreateViews();
	newTexture->SetName(name);

	ResourceStateTracker::AddGlobalResourceState(textureResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	uint32_t bindless3DIndex = (uint32_t)m_loaded3DTextures.size();
	uint32_t bindlessHeapIndex = MAX_NUM_BINDLESS_TEXTURES + MAX_NUM_BINDLESS_STRUCTURED_BUFFERS + bindless3DIndex;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture3D.MostDetailedMip = 0;
	srvDesc.Texture3D.MipLevels = 1;
	srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;

	newTexture->CreateBindlessSRV(m_bindlessHeap, bindlessHeapIndex, &srvDesc);
	newTexture->SetBindlessIndex(bindless3DIndex);

	m_loaded3DTextures.push_back(newTexture);
	return newTexture;
}



BitmapFont* RendererDX12::CreateOrGetBitMapFontFromFile(char const* bitmapFontFilePathWithNoExtension)
{
	BitmapFont* existingBitMapFont = GetBitMapFontForFileName(bitmapFontFilePathWithNoExtension);
	if (existingBitMapFont)
	{
		return existingBitMapFont;
	}

	BitmapFont* newBitMapFont = nullptr;
	//#TODO: need to remove the adding file extension line
	std::string textureFilePath = Stringf("%s.png", bitmapFontFilePathWithNoExtension);
#ifdef RENDERER_DX12

	TextureDX12* newTexture = CreateOrGetTextureFromFile(textureFilePath.c_str());
	newBitMapFont = new BitmapFont(bitmapFontFilePathWithNoExtension, *newTexture, IntVec2(16, 16));
	m_loadedFonts.push_back(newBitMapFont);

#endif 

	return newBitMapFont;
}

TextureDX12* RendererDX12::GetTextureForFileName(char const* imageFilePath, int numMipLevels) const
{
	std::string fileName = GetFileName(imageFilePath);
	if (numMipLevels > 0)
	{
		fileName += Stringf("_Mips:%i", numMipLevels);
	}

	for (int i = 0; i < (int)m_loadedTextures.size(); ++i)
	{
		std::string textureName = m_loadedTextures[i]->GetName();
		if (textureName == fileName)
		{
			return m_loadedTextures[i];
		}
	}

	return nullptr;
}

bool RendererDX12::IsTextureLoaded(std::string const& filePath, TextureDX12*& out_texture, int numMipLevels)
{
	out_texture = GetTextureForFileName(filePath.c_str(), numMipLevels);
	if (out_texture)
	{
		return true;
	}

	return false;
}

//creation
TextureDX12* RendererDX12::CreateTextureFromFile(char const* imageFilePath)
{
	if ((int)m_loadedTextures.size() >= MAX_NUM_BINDLESS_TEXTURES)
	{
		ERROR_AND_DIE(Stringf("Number of bindless textures exceeded maximum: %i", MAX_NUM_BINDLESS_TEXTURES));
	}

	std::string fileName = GetFileName(imageFilePath);
	TextureDX12* newTexture = new TextureDX12(this, fileName, nullptr);
	m_directCommandList->LoadTextureFromFile(*newTexture, imageFilePath);

	uint32_t bindlessIndex = (uint32_t)m_loadedTextures.size();
	newTexture->CreateBindlessSRV(m_bindlessHeap, bindlessIndex);
	newTexture->SetBindlessIndex(bindlessIndex);

	m_loadedTextures.push_back(newTexture);
 	return newTexture;
}

TextureDX12* RendererDX12::CreateTextureFromImage(Image const& image)
{
	if ((int)m_loadedTextures.size() >= MAX_NUM_BINDLESS_TEXTURES)
	{
		ERROR_AND_DIE(Stringf("Number of bindless textures exceeded maximum: %i", MAX_NUM_BINDLESS_TEXTURES));
	}

	std::string imageName = GetFileName(image.GetImageFilePath());
	TextureDX12* newTexture = new TextureDX12(this, imageName, nullptr);

	m_directCommandList->LoadTextureFromImage(*newTexture, imageName, image);

	uint32_t bindlessIndex = (uint32_t)m_loadedTextures.size();
	newTexture->CreateBindlessSRV(m_bindlessHeap, bindlessIndex);
	newTexture->SetBindlessIndex(bindlessIndex);

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

TextureDX12* RendererDX12::CreateTextureFromFileWithMipMaps(char const* imageFilePath, int numMipLevels)
{
	if ((int)(m_loadedTextures.size()) >= MAX_NUM_BINDLESS_TEXTURES)
	{
		ERROR_AND_DIE(Stringf("Number of bindless textures exceeded maximum: %i", MAX_NUM_BINDLESS_TEXTURES));
	}

	std::string fileName = GetFileName(imageFilePath) + Stringf("_Mips:%i", numMipLevels);

	TextureDX12* newTexture = new TextureDX12(this, fileName, nullptr);

	// Record upload on copy list (copy queue).
	GUARANTEE_OR_DIE((m_copyCommandList != nullptr), "Copy command list is null");
	m_copyCommandList->LoadTextureFromFileWithMips(*newTexture, imageFilePath, numMipLevels);

	// Create bindless SRV right away (it should include all mips, once generated).
	uint32_t bindlessIndex = (uint32_t)(m_loadedTextures.size());
	newTexture->CreateBindlessSRV(m_bindlessHeap, bindlessIndex);
	newTexture->SetBindlessIndex(bindlessIndex);

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

TextureDX12* RendererDX12::CreateTextureFromImageWithMipMaps(Image const& image, int numMipLevels)
{
	if ((int)(m_loadedTextures.size()) >= MAX_NUM_BINDLESS_TEXTURES)
	{
		ERROR_AND_DIE(Stringf("Number of bindless textures exceeded maximum: %i", MAX_NUM_BINDLESS_TEXTURES));
	}

	std::string imageName = GetFileName(image.GetImageFilePath()) + Stringf("_Mips:%i", numMipLevels);

	TextureDX12* newTexture = new TextureDX12(this, imageName, nullptr);

	// Record upload on copy list (copy queue).
	GUARANTEE_OR_DIE((m_copyCommandList != nullptr), "Copy command list is null");
	m_copyCommandList->LoadTextureFromImageWithMips(*newTexture, imageName, image, numMipLevels);

	// Create bindless SRV right away (it should include all mips, once generated).
	uint32_t bindlessIndex = (uint32_t)(m_loadedTextures.size());
	newTexture->CreateBindlessSRV(m_bindlessHeap, bindlessIndex);
	newTexture->SetBindlessIndex(bindlessIndex);

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

uint32_t RendererDX12::ComputeMipDimension(uint32_t baseDimension, uint32_t mipIndex)
{
	uint32_t dimension = (baseDimension >> mipIndex);
	if (dimension < 1) { dimension = 1; }
	return dimension;
}

void RendererDX12::GenerateMipsCompute(TextureDX12* texture, IntVec2 const& baseDimensions, uint16_t numMipLevels)
{
	if (texture == nullptr) { return; }
	if (m_computeCommandList == nullptr) { return; }
	if (numMipLevels <= 1) { return; }



	BindComputeRootSignature(m_defaultComputeRootSignature);
	BindComputeShader(m_mipMapComputeShader);
	SetComputeStatesIfChanged();

	ID3D12Resource* textureResource = texture->GetD3D12Resource();
	D3D12_RESOURCE_DESC resourceDesc = textureResource->GetDesc();

	uint16_t resourceMipLevels = resourceDesc.MipLevels;
	if (numMipLevels > resourceMipLevels)
	{
		numMipLevels = resourceMipLevels;
	}

	if (numMipLevels <= 1)
	{
		return;
	}

	uint32_t baseWidth = (uint32_t)(baseDimensions.x);
	uint32_t baseHeight = (uint32_t)(baseDimensions.y);

	DXGI_FORMAT viewFormat = TextureDX12::GetUAVCompatableFormat(resourceDesc.Format);

	// Make sure mip 0 can be read by compute.
	m_computeCommandList->TransitionBarrier(*texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0, false);
	m_computeCommandList->FlushResourceBarriers();

	for (uint16_t sourceMipIndex = 0; (sourceMipIndex + 1) < numMipLevels; sourceMipIndex = (uint16_t)(sourceMipIndex + 1))
	{
		uint16_t destinationMipIndex = (uint16_t)(sourceMipIndex + 1);

		uint32_t sourceMipWidth = ComputeMipDimension(baseWidth, (uint32_t)(sourceMipIndex));
		uint32_t sourceMipHeight = ComputeMipDimension(baseHeight, (uint32_t)(sourceMipIndex));
		uint32_t destinationMipWidth = ComputeMipDimension(baseWidth, (uint32_t)(destinationMipIndex));
		uint32_t destinationMipHeight = ComputeMipDimension(baseHeight, (uint32_t)(destinationMipIndex));

		// Transition states for this step.
		m_computeCommandList->TransitionBarrier(*texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, (unsigned int)(sourceMipIndex), false);
		m_computeCommandList->TransitionBarrier(*texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, (unsigned int)(destinationMipIndex), false);
		m_computeCommandList->FlushResourceBarriers();

		// Build a single-mip SRV for the source mip (t0).
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = viewFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = sourceMipIndex;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		D3D12_CPU_DESCRIPTOR_HANDLE sourceMipSrvHandle = texture->GetShaderResourceView(&srvDesc);

		// Build a single-mip UAV for the destination mip (u0).
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = viewFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = destinationMipIndex;
		uavDesc.Texture2D.PlaneSlice = 0;

		// Bind constants (b0).
		MipGenerationConstants constants = {};
		constants.m_destinationMipWidth = destinationMipWidth;
		constants.m_destinationMipHeight = destinationMipHeight;
		constants.m_sourceMipWidth = sourceMipWidth;
		constants.m_sourceMipHeight = sourceMipHeight;

		SetComputeConstantBufferData(0, constants);

		// Bind SRV table (root param 1, t0).
		SetComputeSRVFromView((uint32_t)ComputeRootParameters::SRV, 0, texture, sourceMipSrvHandle, D3D12_RESOURCE_STATE_COMMON, (unsigned int)(sourceMipIndex), 1);
		SetComputeUAV((uint32_t)ComputeRootParameters::UAV, 0, texture, D3D12_RESOURCE_STATE_COMMON, (unsigned int)(destinationMipIndex), 1, &uavDesc);

		// Dispatch.
		uint32_t groupsX = (destinationMipWidth + 7) / 8;
		uint32_t groupsY = (destinationMipHeight + 7) / 8;
		Dispatch(groupsX, groupsY, 1);
		//m_computeCommandList->GetGraphicsCommandList()->Dispatch(groupsX, groupsY, 1);

		// Ensure writes are visible before the next iteration.
		m_computeCommandList->UAVBarrier(*texture, true);

		// Make the mip we just wrote readable for the next iteration as a source SRV.
		m_computeCommandList->TransitionBarrier(*texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, (unsigned int)(destinationMipIndex), true);
	}

	// Final state for sampling in graphics.
	for (uint16_t mipIndex = 0; mipIndex < numMipLevels; mipIndex = (uint16_t)(mipIndex + 1))
	{
		m_computeCommandList->TransitionBarrier(*texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, (unsigned int)(mipIndex), false);
	}
	m_computeCommandList->FlushResourceBarriers();
	m_computeCommandList->FlushResourceBarriers();

	m_computeCommandQueue->QueueResourceForMarkedUse(texture);

}

void RendererDX12::QueueTextureForMipGeneration(TextureDX12* texture, IntVec2 const& dimensions, uint16_t destinationMipCount)
{
	PendingMipGenerationRequest request;
	request.m_texture = texture;
	request.m_baseDimensions = dimensions;
	request.m_mipCount = destinationMipCount;
	m_pendingMipRequests.push_back(request);
}

void RendererDX12::CreateBindlessSRVHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = MAX_NUM_BINDLESS_SRVS;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_bindlessHeap));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create bindless srv heap");

	m_bindlessDescriptorHeapStartGPUHandle = m_bindlessHeap->GetGPUDescriptorHandleForHeapStart();
	m_bindlessDescriptorIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}


//Render Target
//-----------------------------------------------------------------------------------

RenderTarget RendererDX12::GetCopyOfCurrentRenderTarget()
{

	BeginRendererEvent("Copy - RenderTarget");
	RenderTarget copyRT = m_renderTargetCopies[m_currentRenderTargetCopyIndex];
	m_currentRenderTargetCopyIndex = (m_currentRenderTargetCopyIndex + 1) % NUM_COPY_RENDER_TARGETS;
#ifdef RENDERER_DX12

	ID3D12GraphicsCommandList* cmd = m_directCommandList->GetGraphicsCommandList();

	// SOURCE
	TextureDX12 const* srcColor = m_renderTarget.m_color;
	TextureDX12 const* srcDepth = m_renderTarget.m_depth;
	TextureDX12 const* srcMask = m_renderTarget.m_mask;

	// DESTINATION
	TextureDX12 const* dstColor = copyRT.m_color;
	TextureDX12 const* dstDepth = copyRT.m_depth;
	TextureDX12 const* dstMask = copyRT.m_mask;

	// COLOR COPY
	if (srcColor && dstColor)
	{
		D3D12_RESOURCE_BARRIER toCopySource = {};
		toCopySource.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		toCopySource.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		toCopySource.Transition.pResource = srcColor->GetD3D12Resource();
		toCopySource.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		toCopySource.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		toCopySource.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		cmd->ResourceBarrier(1, &toCopySource);

		D3D12_RESOURCE_BARRIER dstToCopyDest = {};
		dstToCopyDest.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		dstToCopyDest.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		dstToCopyDest.Transition.pResource = dstColor->GetD3D12Resource();
		dstToCopyDest.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		dstToCopyDest.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		dstToCopyDest.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		cmd->ResourceBarrier(1, &dstToCopyDest);

		cmd->CopyResource(dstColor->GetD3D12Resource(), srcColor->GetD3D12Resource());

		D3D12_RESOURCE_BARRIER dstBackToSRV = {};
		dstBackToSRV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		dstBackToSRV.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		dstBackToSRV.Transition.pResource = dstColor->GetD3D12Resource();
		dstBackToSRV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		dstBackToSRV.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		dstBackToSRV.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		cmd->ResourceBarrier(1, &dstBackToSRV);

		// Return source back to its previous state
		D3D12_RESOURCE_BARRIER srcBackToRT = {};
		srcBackToRT.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		srcBackToRT.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		srcBackToRT.Transition.pResource = srcColor->GetD3D12Resource();
		srcBackToRT.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		srcBackToRT.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		srcBackToRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		cmd->ResourceBarrier(1, &srcBackToRT);
	}

	//MASK COPY
	if (srcMask && dstMask)
	{
		D3D12_RESOURCE_BARRIER toCopySource = {};
		toCopySource.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		toCopySource.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		toCopySource.Transition.pResource = srcMask->GetD3D12Resource();
		toCopySource.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		toCopySource.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		toCopySource.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		cmd->ResourceBarrier(1, &toCopySource);

		D3D12_RESOURCE_BARRIER dstToCopyDest = {};
		dstToCopyDest.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		dstToCopyDest.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		dstToCopyDest.Transition.pResource = dstMask->GetD3D12Resource();
		dstToCopyDest.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		dstToCopyDest.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		dstToCopyDest.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		cmd->ResourceBarrier(1, &dstToCopyDest);

		cmd->CopyResource(dstMask->GetD3D12Resource(), srcMask->GetD3D12Resource());

		D3D12_RESOURCE_BARRIER dstBackToSRV = {};
		dstBackToSRV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		dstBackToSRV.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		dstBackToSRV.Transition.pResource = dstMask->GetD3D12Resource();
		dstBackToSRV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		dstBackToSRV.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		dstBackToSRV.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		cmd->ResourceBarrier(1, &dstBackToSRV);

		D3D12_RESOURCE_BARRIER srcBackToRT = {};
		srcBackToRT.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		srcBackToRT.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		srcBackToRT.Transition.pResource = srcMask->GetD3D12Resource();
		srcBackToRT.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		srcBackToRT.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		srcBackToRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		cmd->ResourceBarrier(1, &srcBackToRT);
	}

	// DEPTH COPY
	if (srcDepth && dstDepth)
	{
		D3D12_RESOURCE_BARRIER depthSrcToCopy = {};
		depthSrcToCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthSrcToCopy.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthSrcToCopy.Transition.pResource = srcDepth->GetD3D12Resource();
		depthSrcToCopy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		depthSrcToCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		depthSrcToCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		cmd->ResourceBarrier(1, &depthSrcToCopy);

		D3D12_RESOURCE_BARRIER depthDstToCopy = {};
		depthDstToCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthDstToCopy.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthDstToCopy.Transition.pResource = dstDepth->GetD3D12Resource();
		depthDstToCopy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		depthDstToCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		depthDstToCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		cmd->ResourceBarrier(1, &depthDstToCopy);

		cmd->CopyResource(dstDepth->GetD3D12Resource(), srcDepth->GetD3D12Resource());

		D3D12_RESOURCE_BARRIER depthDstBackToSRV = {};
		depthDstBackToSRV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthDstBackToSRV.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthDstBackToSRV.Transition.pResource = dstDepth->GetD3D12Resource();
		depthDstBackToSRV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		depthDstBackToSRV.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		depthDstBackToSRV.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		cmd->ResourceBarrier(1, &depthDstBackToSRV);

		D3D12_RESOURCE_BARRIER depthSrcBackToWrite = {};
		depthSrcBackToWrite.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthSrcBackToWrite.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthSrcBackToWrite.Transition.pResource = srcDepth->GetD3D12Resource();
		depthSrcBackToWrite.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		depthSrcBackToWrite.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		depthSrcBackToWrite.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		cmd->ResourceBarrier(1, &depthSrcBackToWrite);
	}
#endif // RENDERER_DX12

	EndRendererEvent();
	return copyRT;
}

TextureDX12* RendererDX12::CreateRenderTexture(IntVec2 const& dimensions)
{
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = dimensions.x;
	desc.Height = dimensions.y;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;

	TextureDX12* tex = new TextureDX12(this,"RenderTarget_Color",desc,&clearValue);

	tex->SetDimensions(dimensions);
	return tex;
}

TextureDX12* RendererDX12::CreateDepthTexture(IntVec2 const& dimensions)
{
	DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = dimensions.x;
	desc.Height = dimensions.y;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = depthFormat; // will create DSV
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = depthFormat;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	TextureDX12* tex = new TextureDX12(this,"RenderTarget_Depth",desc,&clearValue);

	tex->SetDimensions(dimensions);
	return tex;
}

RenderTarget RendererDX12::CreateRenderTarget(IntVec2 const& dimensions)
{
	RenderTarget renderTarget;
#ifdef RENDERER_DX12
	renderTarget.m_color = CreateRenderTexture(dimensions);
	renderTarget.m_depth = CreateDepthTexture(dimensions);
	renderTarget.m_mask = CreateRenderTexture(dimensions);
#else
	UNUSED(dimensions);
#endif // RENDERER_DX12

	return renderTarget;
}

RenderTarget RendererDX12::CreateCopyRenderTarget(IntVec2 const& dimensions)
{
	RenderTarget copyRT;
#ifdef RENDERER_DX12
	copyRT.m_color = CreateCopyRenderTargetColor(dimensions);
	copyRT.m_depth = CreateCopyRenderTargetDepth(dimensions);
	copyRT.m_mask = CreateCopyRenderTargetColor(dimensions);
#else
	UNUSED(dimensions);
#endif // RENDERER_DX12

	return copyRT;
}

TextureDX12* RendererDX12::CreateCopyRenderTargetColor(IntVec2 const& dimensions)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = dimensions.x;
	desc.Height = dimensions.y;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// Create wrapper texture
	TextureDX12* tex = new TextureDX12(this,"RenderTargetCopy_Color",desc,nullptr);

	// Allocate a bindless SRV for this texture
	uint32_t bindlessIndex = (uint32_t)m_loadedTextures.size();
	tex->CreateBindlessSRV(m_bindlessHeap, bindlessIndex,nullptr);
	tex->SetBindlessIndex(bindlessIndex);
	m_loadedTextures.push_back(tex);

	return tex;
}

TextureDX12* RendererDX12::CreateCopyRenderTargetDepth(IntVec2 const& dimensions)
{
	DXGI_FORMAT typelessFormat = DXGI_FORMAT_R32_TYPELESS;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = dimensions.x;
	desc.Height = dimensions.y;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = typelessFormat;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE; // not a real depth buffer

	TextureDX12* tex = new TextureDX12(
		this,
		"DepthCopyTexture",
		desc,
		nullptr
	);

	tex->SetDimensions(dimensions);

	// Create an SRV view of R32_FLOAT
	D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
	srv.Format = DXGI_FORMAT_R32_FLOAT;
	srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv.Texture2D.MostDetailedMip = 0;
	srv.Texture2D.MipLevels = 1;

	uint32_t bindlessIndex = (uint32_t)m_loadedTextures.size();
	tex->CreateBindlessSRV(m_bindlessHeap, bindlessIndex, &srv);
	tex->SetBindlessIndex(bindlessIndex);

	m_loadedTextures.push_back(tex);
	return tex;
}

//Clear Commands
//-----------------------------------------------------------------------------------
void RendererDX12::ClearRTV(D3D12_CPU_DESCRIPTOR_HANDLE rtv, float* clearColor) const
{
	ID3D12GraphicsCommandList* d3d12CommandList = m_directCommandList->GetGraphicsCommandList();
	d3d12CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void RendererDX12::ClearDepthStencilView( D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth) const
{
	ID3D12GraphicsCommandList* d3d12CommandList = m_directCommandList->GetGraphicsCommandList();
	d3d12CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void RendererDX12::ClearScreen(Rgba8 const& clearColor)
{
	float clearColorFloat[4] = {};
	clearColor.GetAsFloats(clearColorFloat);
#ifdef RENDERER_DX12


	if (m_activeRenderTarget)
	{
		ClearRTV(m_renderTarget.m_color->GetRenderTargetView(), clearColorFloat);
		if (m_renderTarget.m_mask)
		{
			float maskClearColor[4] = { 0.f, 0.f, 0.f, 0.f };
			ClearRTV(m_renderTarget.m_mask->GetRenderTargetView(), maskClearColor);
		}
	}

	else
	{
		ClearRTV(GetBackBufferRenderTargetView(), clearColorFloat);
	}
#endif // RENDERER_DX12
}


void RendererDX12::ClearDepth()
{
#ifdef RENDERER_DX12
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_renderTarget.m_depth->GetDepthStencilView();
	ClearDepthStencilView(dsv);
#endif // RENDERER_DX12
}



//Camera
//-----------------------------------------------------------------------------------
void RendererDX12::BeginCamera(Camera const& camera)
{
	Vec2 screenBottomLeft = camera.GetOrthoBottomLeft();
	Vec2 screenTopRight = camera.GetOrthoTopRight();

	Vec2 clientDims((float)g_window->GetClientDimensions().x, (float)g_window->GetClientDimensions().y);
	AABB2 viewportNormalizedBounds = camera.GetViewportBounds();
	float viewportWidth = (viewportNormalizedBounds.m_maxs.x - viewportNormalizedBounds.m_mins.x) * clientDims.x;
	float viewportHeight = (viewportNormalizedBounds.m_maxs.y - viewportNormalizedBounds.m_mins.y) * clientDims.y;

	float topLeftX = viewportNormalizedBounds.m_mins.x * clientDims.x;
	float topLeftY = (1.f - viewportNormalizedBounds.m_maxs.y) * clientDims.y;
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = topLeftX;
	viewport.TopLeftY = topLeftY;
	viewport.Width = viewportWidth;
	viewport.Height = viewportHeight;
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;

	m_directCommandList->SetViewport(viewport);

	D3D12_RECT scissorRect = { 0,0, (LONG)viewportWidth, (LONG)viewportHeight };
	m_directCommandList->SetScissorRect(scissorRect);


	CameraConstants cameraConstants;
	cameraConstants.m_worldToCameraTransform = camera.GetWorldToCameraTransform();
	cameraConstants.m_cameraToRenderTransform = camera.GetCameraToRenderTransform();
	cameraConstants.m_renderToClipTransform = camera.GetRenderToClipTransform();
	cameraConstants.m_cameraPosition = camera.GetPosition();
	SetConstantBufferData((uint32_t)RootParameters::CAMERA_CONSTANTS, cameraConstants);

	SetModelConstants(Mat44::IDENTITY, Rgba8::WHITE);

	ColorAdjustmentConstants colorConstants;
	colorConstants.m_saturation = 1.f;
	SetColorAdjustmentConstants(colorConstants);
}

void RendererDX12::EndCamera(Camera const& camera)
{
	UNUSED(camera);
}

//Renderer Events
//-----------------------------------------------------------------------------------

void RendererDX12::BeginRendererEvent(char const* eventName) const
{
	m_directCommandList->BeginRendererEvent(eventName);
}

void RendererDX12::EndRendererEvent() const
{
	m_directCommandList->EndRendererEvent();
}

//Binding
//---------------------------------------------------------------------------------------

void RendererDX12::BindGraphicsDescriptorHeaps()
{
	ID3D12GraphicsCommandList* d3d12CommandList = m_directCommandList->GetGraphicsCommandList();

	ID3D12DescriptorHeap* heaps[] = { m_bindlessHeap };
	d3d12CommandList->SetDescriptorHeaps(_countof(heaps), heaps);
	d3d12CommandList->SetGraphicsRootDescriptorTable((unsigned int)RootParameters::TEXTURES, m_bindlessDescriptorHeapStartGPUHandle);
}


void RendererDX12::BindPSO(PipelineStateObjectDX12 const* pipelineStateObject)
{
	m_desiredPSO = pipelineStateObject;
}

void RendererDX12::BindRootSignature(RootSignatureDX12* rootSignature)
{
	if (rootSignature)
	{
		m_desiredRootSignature = rootSignature;
	}

	else
	{
		m_desiredRootSignature = m_defaultRootSignature;
	}
}

void RendererDX12::BindTexture(TextureDX12 const* texture, int slot)
{
	if (texture)
	{
		m_currentTextureConstants.m_textureIndexes[slot] = (uint32_t)texture->GetBindlessIndex();
	}

	else
	{
		m_currentTextureConstants.m_textureIndexes[slot] = m_defaultTextureConstants.m_textureIndexes[slot];
	}

	m_uploadTextureConstants = true;
}

void RendererDX12::BindShader(ShaderDX12 const* shader)
{
	if (shader)
	{
		m_desiredShader = shader;
	}

	else
	{
		m_desiredShader = m_defaultShader;
	}
}

void RendererDX12::BindStructuredBuffer(StructuredBufferDX12* buffer, int slot)
{
	if(buffer)
	{
		m_currentStructuredBufferConstants.m_bufferIndexes[slot] = buffer->GetBindlessIndex();
		m_uploadStructuredBufferConstants = true;
	}

	else
	{
		m_currentStructuredBufferConstants.m_bufferIndexes[slot] = m_defaultStructuredBufferIndex;
		m_uploadStructuredBufferConstants = true;
	}
}

void RendererDX12::BindComputePSO(PipelineStateObjectDX12 const* pso)
{
	if(!pso)
		ERROR_AND_DIE("Binding Null ComputePSO");

	if(!pso->m_shader->IsComputeShader())
		ERROR_AND_DIE("Binding non-compute PSO to m_desiredComputePSO");

	m_desiredComputePSO = pso;
}

void RendererDX12::BindComputeRootSignature(RootSignatureDX12* rootSignature)
{
	if (!rootSignature)
	{
		m_desiredComputeRootSignature = m_defaultComputeRootSignature;
	}

	else
	{
		m_desiredComputeRootSignature = rootSignature;
	}
}

void RendererDX12::BindComputeShader(ShaderDX12 const* shader)
{
	if(!shader)
		ERROR_AND_DIE("Binding Null ComputeShader");
	if(!shader->IsComputeShader())
		ERROR_AND_DIE("Binding non-compute shader to Compute Shader");

	m_desiredComputeShader = shader;
}

//Drawing
//---------------------------------------------------------------------------------------
void RendererDX12::SetStatesIfChanged()
{
	//Only bind new objects if they have changed since the last draw call
	if (!m_desiredPSO) //allows to bind PSO's directly or to bind by piecing together different states
	{
		if (m_desiredDepthMode != m_currentDepthMode)
		{
			m_currentDepthMode = m_desiredDepthMode;
		}

		if (m_desiredBlendMode != m_currentBlendMode)
		{
			m_currentBlendMode = m_desiredBlendMode;
		}

		if (m_desiredRasterizerMode != m_currentRasterizerMode)
		{
			m_currentRasterizerMode = m_desiredRasterizerMode;
		}

		if (m_desiredShader != m_currentShader)
		{
			m_currentShader = m_desiredShader;
		}

		//#TODO: add sampler mode check here

		if (m_desiredRootSignature != m_currentRootSignature)
		{
			if (!m_desiredRootSignature)
			{
				m_directCommandList->SetGraphicsRootSignature(*m_defaultRootSignature);
				m_currentRootSignature = m_defaultRootSignature;
			}

			else
			{
				m_directCommandList->SetGraphicsRootSignature(*m_desiredRootSignature);
				m_currentRootSignature = m_desiredRootSignature;
			}
		}

		m_desiredPSO = CreateOrGetPSO(m_currentShader, m_desiredRootSignature, m_currentBlendMode, m_currentDepthMode, m_currentRasterizerMode, m_activeRenderTarget);
	}

	else
	{
		if (m_desiredRootSignature != m_currentRootSignature)
		{
			if (!m_desiredRootSignature)
			{
				m_directCommandList->SetGraphicsRootSignature(*m_defaultRootSignature);
				m_currentRootSignature = m_defaultRootSignature;

			}

			else
			{
				m_directCommandList->SetGraphicsRootSignature(*m_desiredRootSignature);
				m_currentRootSignature = m_desiredRootSignature;
			}
		}
	}

	if (m_desiredPSO != m_currentPSO)
	{
		m_currentPSO = m_desiredPSO;
	}

	if (m_rebindGraphicsDescriptorHeap)
	{
		BindGraphicsDescriptorHeaps();
		m_rebindGraphicsDescriptorHeap = false;
	}

	
	m_directCommandList->SetPipelineState(m_currentPSO->m_pipelineState);
	

	m_desiredPSO = nullptr;

	//Texture constants allows certain shaders to act similarly to slots even with bindless texturing.
	//Some shaders do not need this to be uploaded
	if(m_uploadTextureConstants)
	{
		SetBindlessTextureConstants(m_currentTextureConstants);
		m_uploadTextureConstants = false;
	}

	if (m_uploadStructuredBufferConstants)
	{
		SetBindlessStructuredBufferConstants(m_currentStructuredBufferConstants);
		m_uploadStructuredBufferConstants = false;
	}
}

void RendererDX12::SetComputeStatesIfChanged(D3D12_COMMAND_LIST_TYPE commandListType)
{
	//if the last compute call was not on the same command list type, PSO and Root Signature need to be rebound
	if (m_lastComputeCommandListType != commandListType)
	{
		m_currentComputePSO = nullptr;
		m_currentComputeRootSignature = nullptr;
	}

	m_lastComputeCommandListType = commandListType;

	CommandList* commandList = GetCurrentCommandList(commandListType);
	GUARANTEE_OR_DIE((commandList != nullptr), "Compute command list is null");
	if(!m_desiredComputeRootSignature)
	{
		m_desiredComputeRootSignature = m_defaultComputeRootSignature;
	}

	if (!m_desiredComputePSO)
	{
		m_desiredComputePSO = CreateOrGetPSO(m_desiredComputeShader, m_desiredComputeRootSignature);
	}

	if (m_desiredComputeRootSignature != m_currentComputeRootSignature)
	{
		commandList->SetComputeRootSignature(*m_desiredComputeRootSignature);
		m_currentComputeRootSignature = m_desiredComputeRootSignature;
	}

	if (m_desiredComputePSO != m_currentComputePSO)
	{
		commandList->SetPipelineState(m_desiredComputePSO->m_pipelineState);
		m_currentComputePSO = m_desiredComputePSO;
	}

	m_desiredComputePSO = nullptr;

	//If the direct command list is used to issue a compute call, descriptor heaps will need to be rebound before the next draw call
	if (commandList == m_directCommandList)
	{
		//m_currentPSO = nullptr;
		//m_currentRootSignature = nullptr;
		m_rebindGraphicsDescriptorHeap = true;
	}
}

void RendererDX12::StageDummyComputeSRV(uint32_t rootParameterIndex, uint32_t descriptorOffset, D3D12_COMMAND_LIST_TYPE commandListType)
{
	CommandList* commandList = GetCurrentCommandList(commandListType);
	commandList->StageShaderResourceViewHandleOnly(rootParameterIndex, descriptorOffset, m_dummySrvHandle);
}

void RendererDX12::DrawVertexArray(Verts const& verts)
{
	SetStatesIfChanged();
	m_directCommandList->SetDynamicVertexBuffer(0, verts);
	m_directCommandList->Draw((uint32_t)verts.size(), 1, 0, 0);
}

void RendererDX12::DrawVertexArray(VertTBNs const& verts)
{
	SetStatesIfChanged();
	m_directCommandList->SetDynamicVertexBuffer(0, verts);
	m_directCommandList->Draw((uint32_t)verts.size(), 1, 0, 0);
}

void RendererDX12::DrawVertexBuffer(VertexBufferDX12* vbo, unsigned int vertexCount)
{
	SetStatesIfChanged();
	m_directCommandList->SetVertexBuffer(0, *vbo);
	m_directCommandList->Draw(vertexCount, 1, 0, 0);

	m_directCommandQueue->QueueResourceForMarkedUse(vbo);
}

void RendererDX12::DrawIndexedVertexBuffer(VertexBufferDX12* vbo, IndexBufferDX12* ibo, unsigned int indexCount)
{
	SetStatesIfChanged();
	m_directCommandList->SetVertexBuffer(0, *vbo);
	m_directCommandList->SetIndexBuffer(*ibo);
	m_directCommandList->DrawIndexed(indexCount, 1, 0, 0, 0);

	m_directCommandQueue->QueueResourceForMarkedUse(vbo);
	m_directCommandQueue->QueueResourceForMarkedUse(ibo);
}

void RendererDX12::DrawInstancedVertexBuffer(VertexBufferDX12* vbo, unsigned int vertexCount, unsigned int instanceCount)
{
	SetStatesIfChanged();
	m_directCommandList->SetVertexBuffer(0, *vbo);
	m_directCommandList->Draw(vertexCount, instanceCount, 0, 0);
	m_directCommandQueue->QueueResourceForMarkedUse(vbo);
}

void RendererDX12::DrawInstancedIndexedVertexBufer(VertexBufferDX12* vbo, IndexBufferDX12* ibo, unsigned int indexCount, unsigned int instanceCount)
{
	SetStatesIfChanged();
	m_directCommandList->SetVertexBuffer(0, *vbo);
	m_directCommandList->SetIndexBuffer(*ibo);
	m_directCommandList->DrawIndexed(indexCount, instanceCount, 0, 0, 0);

	m_directCommandQueue->QueueResourceForMarkedUse(vbo);
	m_directCommandQueue->QueueResourceForMarkedUse(ibo);
}

void RendererDX12::DrawFullScreenQuad(TextureDX12 const* colorTexture, TextureDX12 const* depthTexture, ShaderDX12 const* shader, AABB2 const& viewportBounds)
{
	BeginRendererEvent("Draw - Full Screen Quad");

	AABB2 fullScreenQuad = AABB2(Vec2::ONE * -1.f, Vec2::ONE);

	IntVec2 clientDims = m_config.m_window->GetClientDimensions();
	float minX = (viewportBounds.m_mins.x * (float)clientDims.x);
	float maxX = (viewportBounds.m_maxs.x * (float)clientDims.x);
	float minY = (viewportBounds.m_mins.y * (float)clientDims.y);
	float maxY = (viewportBounds.m_maxs.y * (float)clientDims.y);
	float flippedMinY = ((float)clientDims.y - maxY);
	float flippedMaxY = ((float)clientDims.y - minY);

	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = minX;
	viewport.TopLeftY = flippedMinY;
	viewport.Width = (maxX - minX);
	viewport.Height = (flippedMaxY - flippedMinY);
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;

	m_directCommandList->SetViewport(viewport);
	D3D12_RECT scissorRect = {};
	scissorRect.left = (LONG)minX;
	scissorRect.top = (LONG)flippedMinY;
	scissorRect.right = (LONG)maxX;
	scissorRect.bottom = (LONG)flippedMaxY;

	m_directCommandList->SetScissorRect(scissorRect);

	Verts fullScreenVerts;
	AddVertsForAABB2D(fullScreenVerts, fullScreenQuad, Rgba8::WHITE, AABB2(0.f, 1.f, 1.f, 0.f));
	SetSamplerMode(SamplerMode::POINT_CLAMP);
	SetDepthMode(DepthMode::DISABLED);
	SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	SetBlendMode(BlendMode::OPAQUE);
	BindTexture(colorTexture);
	BindTexture(depthTexture, 1);
	if (shader)
		BindShader(shader);
	else
		BindShader(m_defaultScreenCopyShader);

	DrawVertexArray(fullScreenVerts);
	EndRendererEvent();

}

void RendererDX12::Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, D3D12_COMMAND_LIST_TYPE commandListType)
{
	SetComputeStatesIfChanged(commandListType);
	CommandList* commandList = GetCurrentCommandList(commandListType);
	if (!m_computeSrvWasBoundThisDispatch)
	{
		StageDummyComputeSRV((uint32_t)ComputeRootParameters::SRV, 0, commandListType);
	}

	commandList->Dispatch(groupsX, groupsY, groupsZ);
	m_computeSrvWasBoundThisDispatch = false;
}


//Vertex and Index Buffers
//---------------------------------------------------------------------------------------

VertexBufferDX12* RendererDX12::CreateVertexBuffer(Verts const& verts, std::string const& name, CommandList* commandList)
{
	VertexBufferDX12* newVertexBuffer = new VertexBufferDX12(this, name);
	if (!commandList)
	{
		m_copyCommandList->CopyVertexBuffer(*newVertexBuffer, verts);
		m_copyCommandQueue->QueueResourceForMarkedUse(newVertexBuffer);
	}

	else
	{
		commandList->CopyVertexBuffer(*newVertexBuffer, verts);
		commandList->GetOwningCommandQueue()->QueueResourceForMarkedUse(newVertexBuffer);
	}

	return newVertexBuffer;
}

VertexBufferDX12* RendererDX12::CreateVertexBuffer(VertTBNs const& verts, std::string const& name, CommandList* commandList)
{
	VertexBufferDX12* newVertexBuffer = new VertexBufferDX12(this, name);
	if (!commandList)
	{
		m_copyCommandList->CopyVertexBuffer(*newVertexBuffer, verts);
		m_copyCommandQueue->QueueResourceForMarkedUse(newVertexBuffer);
	}

	else
	{
		commandList->CopyVertexBuffer(*newVertexBuffer, verts);
		commandList->GetOwningCommandQueue()->QueueResourceForMarkedUse(newVertexBuffer);
	}
	return newVertexBuffer;
}

VertexBufferDX12* RendererDX12::CreateVertexBuffer(TerrainVerts const& verts, std::string const& name, CommandList* commandList)
{
	VertexBufferDX12* newVertexBuffer = new VertexBufferDX12(this, name);
	if (!commandList)
	{
		m_copyCommandList->CopyVertexBuffer(*newVertexBuffer, verts);
		m_copyCommandQueue->QueueResourceForMarkedUse(newVertexBuffer);
	}

	else
	{
		commandList->CopyVertexBuffer(*newVertexBuffer, verts);
		commandList->GetOwningCommandQueue()->QueueResourceForMarkedUse(newVertexBuffer);
	}
	return newVertexBuffer;
}

IndexBufferDX12* RendererDX12::CreateIndexBuffer(std::vector<unsigned int> indexes, std::string const& name, CommandList* commandList)
{
	IndexBufferDX12* newIndexBuffer = new IndexBufferDX12(this, name);
	if (!commandList)
	{
		m_copyCommandList->CopyIndexBuffer(*newIndexBuffer, indexes);
		m_copyCommandQueue->QueueResourceForMarkedUse(newIndexBuffer);
	}

	else
	{
		commandList->CopyIndexBuffer(*newIndexBuffer, indexes);
		commandList->GetOwningCommandQueue()->QueueResourceForMarkedUse(newIndexBuffer);
	}

	return newIndexBuffer;
}

StructuredBufferDX12* RendererDX12::CreateEmptyStructuredBuffer(size_t numElements, size_t elementSize, std::string const& name, bool uavWriteAccess)
{
	size_t totalBytes = (numElements * elementSize);

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = (UINT64)totalBytes;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// This is the critical part: compute writes need UAV support.
	if(uavWriteAccess)
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	else
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	StructuredBufferDX12* buffer = new StructuredBufferDX12(this, desc, name);

	int slot = -1;
	for (int i = 0; i < (int)m_loadedStructuredBuffers.size(); ++i)
	{
		if (m_loadedStructuredBuffers[i] == nullptr)
		{
			slot = i;
			break;
		}
	}

	if (slot == -1)
	{
		// IMPORTANT: use >=
		if ((int)m_loadedStructuredBuffers.size() >= MAX_NUM_BINDLESS_STRUCTURED_BUFFERS)
		{
			ERROR_AND_DIE("Exceeded maximum number of bindless structured buffers");
		}

		slot = (int)m_loadedStructuredBuffers.size();
		m_loadedStructuredBuffers.push_back(nullptr);
	}

	buffer->m_bindlessIndex = slot;
	m_loadedStructuredBuffers[slot] = buffer;

	buffer->CreateViews(numElements, elementSize);
	uint32_t heapIndex = (uint32_t)(STRUCTURED_BUFFER_OFFSET + (uint32_t)slot);
	CopySrvToBindlessHeap(heapIndex, buffer->m_srv);

	return buffer;
}

StructuredBufferDX12* RendererDX12::CreateStructuredBuffer(size_t numElements, size_t elementSize, const void* bufferData, bool uavWriteAccess, std::string const& name)
{
	StructuredBufferDX12* newBuffer = CreateEmptyStructuredBuffer(numElements, elementSize, name, uavWriteAccess);
	m_copyCommandList->CopyStructuredBuffer(*newBuffer, numElements, elementSize, bufferData, uavWriteAccess);
	m_copyCommandQueue->QueueResourceForMarkedUse(newBuffer);
	return newBuffer;
}

bool RendererDX12::CanMakeStructuredBuffers(int numBuffersDesired) const
{
	if(numBuffersDesired > MAX_NUM_BINDLESS_STRUCTURED_BUFFERS)
		return false;

	if (m_loadedStructuredBuffers.size() < MAX_NUM_BINDLESS_STRUCTURED_BUFFERS - numBuffersDesired)
		return true;

	int numEmptySlots = 0;

	for (int i = 0; i < m_loadedStructuredBuffers.size(); ++i)
	{
		if (m_loadedStructuredBuffers[i] == nullptr)
		{
			numEmptySlots++;
			if(numEmptySlots >= numBuffersDesired)
				return true;
		}
	}

	return false;
}



void RendererDX12::CopyDataIntoExistingStructuredBuffer(StructuredBufferDX12* buffer, size_t numElements, size_t elementSize, const void* bufferData)
{
	m_copyCommandList->UploadIntoExistingStructuredBuffer(*buffer, numElements, elementSize, bufferData);
}

StructuredBufferViewDX12* RendererDX12::GetStructuredBufferViewFromVertexBuffer(VertexBufferDX12 const* vbo)
{
	unsigned int stride = vbo->GetStride();
	unsigned int count = vbo->GetNumberVerts();

	ID3D12Resource* resource = vbo->GetD3D12Resource();
	return new StructuredBufferViewDX12(this, resource, count, stride);
}


//Shaders
//---------------------------------------------------------------------------------------
ShaderDX12* RendererDX12::CreateOrGetShaderFromFile(char const* shaderName, VertexType vertexType)
{
	std::string shaderString = (std::string)shaderName;
	for (int shaderIndex = 0; shaderIndex < (int)m_loadedShaders.size(); ++shaderIndex)
	{
		if (m_loadedShaders[shaderIndex]->GetName() == shaderString && !m_loadedShaders[shaderIndex]->IsComputeShader())
		{
			return m_loadedShaders[shaderIndex];
		}
	}

	shaderString.append(".hlsl");
	std::string shaderSource;
	FileReadToString(shaderSource, shaderString);
	return CreateShaderDX12(shaderName, shaderSource.c_str(), vertexType);
}

ShaderDX12* RendererDX12::CreateOrGetComputeShaderFromFile(char const* shaderName)
{
	std::string shaderString = (std::string)shaderName;
	for (int i = 0; i < (int)m_loadedShaders.size(); ++i)
	{
		if (m_loadedShaders[i]->GetName() == shaderString && m_loadedShaders[i]->IsComputeShader())
		{
			return m_loadedShaders[i];
		}
	}

	shaderString.append(".hlsl");
	std::string shaderSource;
	FileReadToString(shaderSource, shaderString);
	return CreateComputeShaderDX12(shaderName, shaderSource.c_str());
}

ShaderDX12* RendererDX12::CreateShaderDX12(char const* shaderName, char const* shaderSource, VertexType vertexType)
{
	ShaderConfig shaderConfig;
	shaderConfig.m_name = shaderName;
	shaderConfig.m_computeShader = false;

	DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(ENGINE_DEBUG_RENDERER)
	shaderFlags = D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif

	ShaderDX12* newShader = new ShaderDX12(shaderConfig);
	ID3DBlob* errorBlob = nullptr;


	//Compile Vertex Shader
	HRESULT hr = D3DCompile(shaderSource, strlen(shaderSource), "VertexShader", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VertexMain", "vs_5_1", shaderFlags, 0, &newShader->m_vertexShaderBlob, &errorBlob);
	if (!SUCCEEDED(hr))
	{
		if (errorBlob != NULL)
		{
			DebuggerPrintf((char*)(errorBlob->GetBufferPointer()));
		}
		ERROR_AND_DIE(Stringf("Could not compile vertex shader: %s", shaderName));
	}


	if (errorBlob != NULL)
	{
		DX_SAFE_RELEASE(errorBlob);
	}

	//Compile Pixel Shader
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
	hr = D3DCompile(shaderSource, strlen(shaderSource), "PixelShader", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PixelMain", "ps_5_1", shaderFlags, 0, &newShader->m_pixelShaderBlob, &errorBlob);
	if (!SUCCEEDED(hr))
	{
		if (errorBlob != NULL)
		{
			DebuggerPrintf((char*)(errorBlob->GetBufferPointer()));
		}
		ERROR_AND_DIE(Stringf("Could not compile pixel shader: %s", shaderName));
	}

	if (errorBlob != NULL)
	{
		DX_SAFE_RELEASE(errorBlob);
	}

	//Set Shader vertex type which will be used to make input layout inside CreatePSO()
	newShader->m_vertexType = vertexType;

	m_loadedShaders.push_back(newShader);
	return newShader;
}

ShaderDX12* RendererDX12::CreateShaderDX12(char const* shaderName, VertexType vertexType)
{
	std::string shaderSource = Stringf("Data/Shaders/%s.hlsl", shaderName);
	std::string outString;
	FileReadToString(outString, shaderSource);
	return CreateShaderDX12(shaderName, outString.c_str(), vertexType);
}


ShaderDX12* RendererDX12::CreateComputeShaderDX12(char const* shaderName, char const* shaderSource)
{
	DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(ENGINE_DEBUG_RENDERER)
	shaderFlags = D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif

	ShaderConfig shaderConfig;
	shaderConfig.m_name = shaderName;
	shaderConfig.m_computeShader = true;

	ShaderDX12* newShader = new ShaderDX12(shaderConfig);

	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3DCompile(shaderSource,strlen(shaderSource),shaderName,nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,"CSMain","cs_5_1",shaderFlags,0,&newShader->m_computeShaderBlob,&errorBlob);

	if (!SUCCEEDED(hr))
	{
		if (errorBlob != nullptr)
		{
			DebuggerPrintf((char*)errorBlob->GetBufferPointer());
			DX_SAFE_RELEASE(errorBlob);
		}
		ERROR_AND_DIE(Stringf("Could not compile compute shader: %s", shaderName));
	}

	if (errorBlob != nullptr)
	{
		DX_SAFE_RELEASE(errorBlob);
	}

	m_loadedShaders.push_back(newShader);
	return newShader;
}

ShaderDX12* RendererDX12::CreateComputeShaderDX12(char const* shaderName)
{
	std::string shaderPath = Stringf("Data/Shaders/%s.hlsl", shaderName);

	std::string shaderSource;
	FileReadToString(shaderSource, shaderPath);

	return CreateComputeShaderDX12(shaderName, shaderSource.c_str());
}

//PSOs
//---------------------------------------------------------------------------------------

PipelineStateObjectDX12* RendererDX12::CreateOrGetPSO(ShaderDX12 const* shader, RootSignatureDX12* rootSignature, BlendMode blendMode, DepthMode depthMode, RasterizerMode rasterizerMode, bool activeRenderTarget)
{
	ShaderDX12 const* usedShader = shader;
	if (!usedShader)
	{
		usedShader = m_defaultShader;
	}

	bool computePSO = usedShader->IsComputeShader();

	RootSignatureDX12* usedRootSignature = rootSignature;
	if (!usedRootSignature)
	{
		usedRootSignature = m_defaultRootSignature; //#TODO: add check for compute shader
	}

	std::string psoName = computePSO ? GetComputePSOName(usedShader, usedRootSignature) : GetGraphicsPSOName(usedShader, usedRootSignature, blendMode, depthMode, rasterizerMode, activeRenderTarget);
	for (int i = 0; i < (int)m_loadedPipelineStateObjects.size(); ++i)
	{
		if (m_loadedPipelineStateObjects[i]->GetName() == psoName)
		{
			return m_loadedPipelineStateObjects[i];
		}
	}

	if(computePSO)
		return CreateComputePSO(usedShader, usedRootSignature);

	return CreateGraphicsPSO(usedShader, usedRootSignature, blendMode, depthMode, rasterizerMode, activeRenderTarget);
}

void RendererDX12::ClearStructuredBufferAtIndex(int index)
{
	m_loadedStructuredBuffers[index] = nullptr;	
}

void RendererDX12::SetComputeSRV(uint32_t rootParameterIndex, uint32_t descriptorOffset, ResourceDX12* resource, D3D12_RESOURCE_STATES stateAfter, unsigned int firstSubresource, unsigned int numSubresources, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv, D3D12_COMMAND_LIST_TYPE commandListType)
{
	CommandList* commandList = GetCurrentCommandList(commandListType);
	CommandQueue* commandQueue = GetCommandQueue(commandListType);

	commandList->SetShaderResourceView(rootParameterIndex, descriptorOffset, *resource, stateAfter, firstSubresource, numSubresources, srv);
	commandQueue->QueueResourceForMarkedUse(resource);
	m_computeSrvWasBoundThisDispatch = true;
}

void RendererDX12::SetComputeSRVFromView(uint32_t rootParameterIndex, uint32_t descriptorOffset, ResourceDX12* resource, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, D3D12_RESOURCE_STATES stateAfter, unsigned int firstSubresource, unsigned int numSubresources)
{
	m_computeCommandList->TransitionBarrier(*resource, stateAfter);
	m_computeCommandList->SetShaderResourceViewHandle(rootParameterIndex, descriptorOffset, *resource, srvHandle, stateAfter, firstSubresource, numSubresources);
	m_computeCommandQueue->QueueResourceForMarkedUse(resource);
	m_computeSrvWasBoundThisDispatch = true;
}

void RendererDX12::SetComputeUAV(uint32_t rootParameterIndex, uint32_t descriptorOffset, ResourceDX12* resource, D3D12_RESOURCE_STATES stateAfter, unsigned int firstSubresource, unsigned int numSubresources, D3D12_UNORDERED_ACCESS_VIEW_DESC const* uav, D3D12_COMMAND_LIST_TYPE commandListType)
{
	CommandList* commandList = GetCurrentCommandList(commandListType);
	SetComputeStatesIfChanged(commandListType);
	commandList->SetUnorderedAccessView(rootParameterIndex, descriptorOffset, *resource, stateAfter, firstSubresource, numSubresources, uav);
	CommandQueue* commandQueue = GetCommandQueue(commandListType);
	commandQueue->QueueResourceForMarkedUse(resource);
}

void RendererDX12::TransitionResource(ResourceDX12& resource, D3D12_RESOURCE_STATES stateAfter, D3D12_COMMAND_LIST_TYPE commandListType)
{
	CommandList* commandList = GetCurrentCommandList(commandListType);
	commandList->UAVBarrier(resource, false);
	commandList->TransitionBarrier(resource, stateAfter, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
}


PipelineStateObjectDX12* RendererDX12::CreateGraphicsPSO(ShaderDX12 const* shader, RootSignatureDX12* rootSignature, BlendMode blendMode, DepthMode depthMode, RasterizerMode rasterizerMode, bool activeRenderTarget)
{
	std::string psoName = GetGraphicsPSOName(shader, rootSignature, blendMode, depthMode, rasterizerMode, activeRenderTarget);
	if (g_devConsole)
	{
		//g_devConsole->AddLine(DevConsole::WARNING,Stringf("New %s", psoName.c_str()),0.5f, true);
	}

	PipelineStateObjectDX12* newPSO = new PipelineStateObjectDX12(shader, rootSignature, psoName);

	DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT maskFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	if (activeRenderTarget)
	{
		rtvFormats.NumRenderTargets = 2;
		rtvFormats.RTFormats[0] = colorFormat;
		rtvFormats.RTFormats[1] = maskFormat;
	}
	else
	{
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = colorFormat;
	}

	//Create Input Layout
	//----------------------------------------------------------------------------------------
	D3D12_INPUT_ELEMENT_DESC inputLayout_VertsPCU[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_ELEMENT_DESC inputLayout_VertsPCUTBN[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	D3D12_INPUT_ELEMENT_DESC inputLayout_VertsTerrain[] = {
	 { "POSITION",      0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL",        0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "SPLAT_INDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT,   0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "SPLAT_WEIGHTS", 0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_INPUT_ELEMENT_DESC* usedInputLayout = nullptr;
	int numInputElements = 0;
	switch (shader->m_vertexType)
	{
	case VertexType::VERTEX_PCU:
		usedInputLayout = inputLayout_VertsPCU;
		numInputElements = _countof(inputLayout_VertsPCU);
		break;
	case VertexType::VERTEX_PCUTBN:
		usedInputLayout = inputLayout_VertsPCUTBN;
		numInputElements = _countof(inputLayout_VertsPCUTBN);
		break;
	case VertexType::VERTEX_TERRAIN:
		usedInputLayout = inputLayout_VertsTerrain;
		numInputElements = _countof(inputLayout_VertsTerrain);
		break;
	default:
		break;
	}

	//Create Blend State description
	//----------------------------------------------------------------------------------------
	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = true;

	D3D12_RENDER_TARGET_BLEND_DESC defaultTargetBlend = {};
	defaultTargetBlend.BlendEnable = false;
	defaultTargetBlend.LogicOpEnable = false;
	defaultTargetBlend.SrcBlend = D3D12_BLEND_ONE;
	defaultTargetBlend.DestBlend = D3D12_BLEND_ZERO;
	defaultTargetBlend.BlendOp = D3D12_BLEND_OP_ADD;
	defaultTargetBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	defaultTargetBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
	defaultTargetBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	defaultTargetBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
	defaultTargetBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	blendDesc.RenderTarget[0] = defaultTargetBlend;
	blendDesc.RenderTarget[1] = defaultTargetBlend;

	D3D12_RENDER_TARGET_BLEND_DESC& colorBlend = blendDesc.RenderTarget[0];
	colorBlend.BlendEnable = true;

	switch (blendMode)
	{
	case BlendMode::OPAQUE:
		colorBlend.SrcBlend = D3D12_BLEND_ONE;
		colorBlend.DestBlend = D3D12_BLEND_ZERO;
		break;
	case BlendMode::ALPHA:
		colorBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		colorBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		break;
	case BlendMode::ADDITIVE:
		colorBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		colorBlend.DestBlend = D3D12_BLEND_ZERO;
		break;
	case BlendMode::LIGHTEN:
		colorBlend.SrcBlend = D3D12_BLEND_ONE;
		colorBlend.DestBlend = D3D12_BLEND_ONE;
		colorBlend.BlendOp = D3D12_BLEND_OP_MAX;
		break;
	default:
		colorBlend.SrcBlend = D3D12_BLEND_ONE;
		colorBlend.DestBlend = D3D12_BLEND_ZERO;
		break;
	}

	//Create DepthStencil description
	//----------------------------------------------------------------------------------------
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;  // usually 0xFF
	depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK; // usually 0xFF
	depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	switch (depthMode)
	{
	case DepthMode::DISABLED:
		depthStencilDesc.DepthEnable = false;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		break;
	case DepthMode::READ_ONLY_ALWAYS:
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		break;
	case DepthMode::READ_ONLY_LESS_EQUAL:
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		break;
	case DepthMode::READ_WRITE_LESS_EQUAL:
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		break;
	default:
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		break;
	}

	//Create rasterizer description
	//----------------------------------------------------------------------------------------
	D3D12_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FrontCounterClockwise = true;
	rasterDesc.DepthClipEnable = true;

	switch (rasterizerMode)
	{
	case RasterizerMode::SOLID_CULL_NONE:
		rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
		break;
	case RasterizerMode::SOLID_CULL_BACK:
		rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
		break;
	case RasterizerMode::SOLID_CULL_FRONT:
		rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterDesc.CullMode = D3D12_CULL_MODE_FRONT;
		break;
	case RasterizerMode::WIREFRAME_CULL_NONE:
		rasterDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
		break;
	case RasterizerMode::WIREFRAME_CULL_BACK:
		rasterDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
		break;
	case RasterizerMode::WIREFRAME_CULL_FRONT:
		rasterDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		rasterDesc.CullMode = D3D12_CULL_MODE_FRONT;
		break;
	default:
		rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
		break;
	}

	DXGI_FORMAT depthFormat;
	if (activeRenderTarget)
	{
		depthFormat = DXGI_FORMAT_D32_FLOAT;  // scene
	}
	else
	{
		depthFormat = DXGI_FORMAT_UNKNOWN;             // direct to back buffer
		depthStencilDesc.DepthEnable = false;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	}


	//Create Pipeline state stream
	//----------------------------------------------------------------------------------------
	PipelineStateStream pipelineStateStream = {};
	pipelineStateStream.RootSignature.m_rootSignature = rootSignature->GetRootSignature();
	pipelineStateStream.InputLayoutDesc.m_inputLayoutDesc.pInputElementDescs = usedInputLayout;
	pipelineStateStream.InputLayoutDesc.m_inputLayoutDesc.NumElements = numInputElements;
	pipelineStateStream.PrimitiveTopologyType.m_typologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VertexShader.m_shaderByteCode.pShaderBytecode = shader->m_vertexShaderBlob->GetBufferPointer();
	pipelineStateStream.VertexShader.m_shaderByteCode.BytecodeLength = shader->m_vertexShaderBlob->GetBufferSize();
	pipelineStateStream.PixelShader.m_shaderByteCode.pShaderBytecode = shader->m_pixelShaderBlob->GetBufferPointer();
	pipelineStateStream.PixelShader.m_shaderByteCode.BytecodeLength = shader->m_pixelShaderBlob->GetBufferSize();
	pipelineStateStream.DepthStencilState.m_desc = depthStencilDesc;
	pipelineStateStream.DepthStencilViewFormat.m_format = depthFormat;
	pipelineStateStream.RenderTargetViewFormats.m_formats = rtvFormats;
	pipelineStateStream.RasterizerState.m_desc = rasterDesc;
	pipelineStateStream.BlendState.m_desc = blendDesc;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };

	//Create PSO
	//----------------------------------------------------------------------------------------
	HRESULT hr = m_device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&newPSO->m_pipelineState));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "could not create pipeline state");

	SetPipelineStateName(newPSO->m_pipelineState, psoName);


	m_loadedPipelineStateObjects.push_back(newPSO);
	return newPSO;
}

PipelineStateObjectDX12* RendererDX12::CreateComputePSO(ShaderDX12 const* computeShader, RootSignatureDX12* rootSignature)
{
	GUARANTEE_OR_DIE((computeShader != nullptr), "Shader was null");
	GUARANTEE_OR_DIE((computeShader->m_computeShaderBlob != nullptr), "Shader is not a compute shader (compute blob is null)");
	GUARANTEE_OR_DIE((rootSignature != nullptr), "Root signature was null");

	std::string psoName = GetComputePSOName(computeShader, rootSignature);
	if (g_devConsole)
	{
		//g_devConsole->AddLine(DevConsole::WARNING, Stringf("New %s", psoName.c_str()), 0.5f, true);
	}
	PipelineStateObjectDX12* newPSO = new PipelineStateObjectDX12(computeShader, rootSignature, psoName);

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = rootSignature->GetRootSignature();
	desc.CS.pShaderBytecode = computeShader->m_computeShaderBlob->GetBufferPointer();
	desc.CS.BytecodeLength = computeShader->m_computeShaderBlob->GetBufferSize();
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hr = m_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&newPSO->m_pipelineState));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create compute pipeline state");

	SetPipelineStateName(newPSO->m_pipelineState, psoName);

	m_loadedPipelineStateObjects.push_back(newPSO);
	return newPSO;
}

std::string RendererDX12::GetGraphicsPSOName(ShaderDX12 const* shader, RootSignatureDX12* rootSignature, BlendMode blendMode, DepthMode depthMode, RasterizerMode rasterizerMode, bool activeRenderTarget) const
{
	std::string shaderName = shader->m_config.m_name;
	std::string rootSigName = rootSignature->GetName();
	std::string blendModeName = GetNameForBlendMode(blendMode);
	std::string depthModeName = GetNameForDepthMode(depthMode);
	std::string rasterizeNodeName = GetNameForRasterizerMode(rasterizerMode);
	std::string psoTypeName = activeRenderTarget ? "RenderTexture" : "BackBuffer";
	psoTypeName = shader->IsComputeShader() ? "Compute" : psoTypeName;
	return Stringf("PSO: S:%s | RS:%s | BM:%s | DM:%s | RM:%s | %s", shaderName.c_str(), rootSigName.c_str(), blendModeName.c_str(), depthModeName.c_str(), rasterizeNodeName.c_str(), psoTypeName.c_str());
}

std::string RendererDX12::GetComputePSOName(ShaderDX12 const* computeShader, RootSignatureDX12* rootSignature) const
{
	std::string shaderName = computeShader->m_config.m_name;
	std::string rootSigName = rootSignature->GetName();
	return Stringf("PSO: S:%s | RS:%s | Compute", shaderName.c_str(), rootSigName.c_str());
}


//Root Signatures
//---------------------------------------------------------------------------------------

D3D12_ROOT_PARAMETER1 RendererDX12::CreateRootConstantBufferParameter(int shaderRegister, int registerSpace)
{
	D3D12_ROOT_PARAMETER1 p = {};
	p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	p.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	p.Descriptor.ShaderRegister = (UINT)shaderRegister;
	p.Descriptor.RegisterSpace = (UINT)registerSpace;
	p.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	return p;
}

RootSignatureDX12* RendererDX12::CreateDefaultBindlessTextureRootSignature()
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));

	if (!SUCCEEDED(hr))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_ROOT_PARAMETER1 rootParams[(int)RootParameters::COUNT] = {};
	//b1 - b16 || parameter 0 - 15
	for (int i = 0; i < NUM_CONSTANT_BUFFERS; ++i)
	{
		int shaderRegister = i + 1; 
		rootParams[i]=CreateRootConstantBufferParameter(shaderRegister, 0);
	}


	//Root Parameter 16 : Descriptor Table for Textures and Structured Buffers (t0)
	D3D12_DESCRIPTOR_RANGE1 srvRange[3] = {};
	//textures: t0-tMAX_BINDLESS_TEXTURES space0
	srvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange[0].NumDescriptors = MAX_NUM_BINDLESS_TEXTURES;
	srvRange[0].BaseShaderRegister = 0; // t0
	srvRange[0].RegisterSpace = 0;
	srvRange[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	srvRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//Structured buffers: t0 - tMAX_BINDLESS_BUFFERS space1. Appended right after textures in the heap
	srvRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange[1].NumDescriptors = MAX_NUM_BINDLESS_STRUCTURED_BUFFERS;
	srvRange[1].BaseShaderRegister = 0;
	srvRange[1].RegisterSpace = 1;
	srvRange[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	srvRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;// MAX_NUM_BINDLESS_TEXTURES + 1;

	
	//3D textures: t0 - tMAX_BINDLESS_3D_TEXTURES space2. Appended right after structured buffers
	srvRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange[2].NumDescriptors = MAX_NUM_BINDLESS_3D_TEXTURES;
	srvRange[2].BaseShaderRegister = 0; // t0
	srvRange[2].RegisterSpace = 2;
	srvRange[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	srvRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	


	D3D12_ROOT_DESCRIPTOR_TABLE1 srvTable = {};
	srvTable.NumDescriptorRanges = 3;
	srvTable.pDescriptorRanges = srvRange;

	D3D12_ROOT_PARAMETER1 textureParam = {};
	textureParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	textureParam.DescriptorTable = srvTable;
	textureParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParams[(int)RootParameters::TEXTURES] = textureParam;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[3] = {};

	for (int i = 0; i < 3; ++i)
	{
		if (i == 0)
		{
			staticSamplers[i].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; //#TODO: this defaults to PointClamp sampler, need to find a way to make this adjustable
			staticSamplers[i].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			staticSamplers[i].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			staticSamplers[i].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		}

		else
		{
			staticSamplers[i].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			staticSamplers[i].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSamplers[i].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSamplers[i].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		}

		staticSamplers[i].MipLODBias = 0.0f;
		staticSamplers[i].MaxAnisotropy = 1;
		staticSamplers[i].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		staticSamplers[i].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		staticSamplers[i].MinLOD = 0.0f;
		staticSamplers[i].MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplers[i].ShaderRegister = i; // buts static sampler in slots s0 - s2
		staticSamplers[i].RegisterSpace = 0;
		staticSamplers[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}


	D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = _countof(rootParams);
	rootSignatureDesc.pParameters = rootParams;
	rootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);
	rootSignatureDesc.pStaticSamplers = staticSamplers;
	rootSignatureDesc.Flags = rootSignatureFlags;

	RootSignatureDX12* newRootSignature = new RootSignatureDX12(this, rootSignatureDesc, featureData.HighestVersion, "Default");
	m_loadedRootSignatures.push_back(newRootSignature);
	return newRootSignature;
}

RootSignatureDX12* RendererDX12::CreateDefaultComputeRootSignature()
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
	if (!SUCCEEDED(hr))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	D3D12_ROOT_PARAMETER1 rootParams[(int)ComputeRootParameters::COUNT] = {};
	//b0 - b15 || parameter 0 - 15
	for (int i = 0; i < NUM_CONSTANT_BUFFERS; ++i)
	{
		int shaderRegister = i;
		rootParams[i] = CreateRootConstantBufferParameter(shaderRegister, 0);
	}

	// RootParam 1: SRV table (t0..t0)
	D3D12_DESCRIPTOR_RANGE1 srvRange = {};
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = 1;
	srvRange.BaseShaderRegister = 0; // t0
	srvRange.RegisterSpace = 0;
	srvRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE1 srvTable = {};
	srvTable.NumDescriptorRanges = 1;
	srvTable.pDescriptorRanges = &srvRange;

	D3D12_ROOT_PARAMETER1 srvParam = {};
	srvParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	srvParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	srvParam.DescriptorTable = srvTable;

	rootParams[(int)ComputeRootParameters::SRV] = srvParam;

	// RootParam 2: UAV table (u0..u0)
	D3D12_DESCRIPTOR_RANGE1 uavRange = {};
	uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	uavRange.NumDescriptors = 1;
	uavRange.BaseShaderRegister = 0; // u0
	uavRange.RegisterSpace = 0;
	uavRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE1 uavTable = {};
	uavTable.NumDescriptorRanges = 1;
	uavTable.pDescriptorRanges = &uavRange;

	D3D12_ROOT_PARAMETER1 uavParam = {};
	uavParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	uavParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	uavParam.DescriptorTable = uavTable;

	rootParams[(int)ComputeRootParameters::UAV] = uavParam;


	D3D12_ROOT_SIGNATURE_DESC1 rootSigDesc = {};
	rootSigDesc.NumParameters = _countof(rootParams);
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 0;
	rootSigDesc.pStaticSamplers = nullptr;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	RootSignatureDX12* newRootSig = new RootSignatureDX12(this, rootSigDesc, featureData.HighestVersion, "Default_Compute");
	m_loadedRootSignatures.push_back(newRootSig);
	return newRootSig;
}



uint64_t RendererDX12::GetLastCompletedFenceValue(D3D12_COMMAND_LIST_TYPE type) const
{
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT: return m_directCommandFence;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_computeCommandFence;
	case D3D12_COMMAND_LIST_TYPE_COPY: return m_copyCommandFence;
	default:
		ERROR_AND_DIE("INVALID COMMAND LIST TYPE");
	}
}



//Constant Buffers
//---------------------------------------------------------------------------------------

void RendererDX12::SetModelConstants(Mat44 const& modelToWorldTransform, Rgba8 const& modelColor)
{
	ModelConstants modelConstants;
	modelConstants.m_modelToWorldTransform = modelToWorldTransform;
	modelColor.GetAsFloats(modelConstants.m_modelColor);

	m_directCommandList->SetGraphicsDynamicConstantBuffer((uint32_t)RootParameters::MODEL_CONSTANTS, modelConstants);
}

void RendererDX12::SetLightConstants(Vec3 const& sunDirection, float sunIntensity, float ambientIntensity, Rgba8 const& sunColor)
{
	LightConstants lightConstants;
	lightConstants.m_sunDirection = sunDirection;
	lightConstants.m_sunIntensity = sunIntensity;
	lightConstants.m_ambientIntensity = ambientIntensity;
	lightConstants.m_sunColorRGB[0] = NormalizeByte(sunColor.r);
	lightConstants.m_sunColorRGB[1] = NormalizeByte(sunColor.g);
	lightConstants.m_sunColorRGB[2] = NormalizeByte(sunColor.b);

	m_directCommandList->SetGraphicsDynamicConstantBuffer((uint32_t)RootParameters::LIGHT_CONSTANTS, lightConstants);
}

void RendererDX12::SetLightConstants(LightConstants const& lightConstants)
{
	m_directCommandList->SetGraphicsDynamicConstantBuffer((uint32_t)RootParameters::LIGHT_CONSTANTS, lightConstants);
}

void RendererDX12::SetColorAdjustmentConstants(ColorAdjustmentConstants const& colorAdjustmentConstants)
{
	m_directCommandList->SetGraphicsDynamicConstantBuffer((uint32_t)RootParameters::CBV_7, colorAdjustmentConstants);
}

void RendererDX12::SetPerFrameConstants(PerFrameConstants const& perFrameConstants)
{
	m_directCommandList->SetGraphicsDynamicConstantBuffer((uint32_t)RootParameters::PER_FRAME_CONSTANTS, perFrameConstants);
}

void RendererDX12::SetBindlessTextureConstants(BindlessTextureConstants const& textureConstants)
{
	m_directCommandList->SetGraphicsDynamicConstantBuffer((uint32_t)RootParameters::BINDLESS_TEXTURE_CONSTANTS, textureConstants);
}

void RendererDX12::SetBindlessStructuredBufferConstants(BindlessStructuredBufferConstants const& bufferConstants)
{
	m_directCommandList->SetGraphicsDynamicConstantBuffer((uint32_t)RootParameters::BINDLESS_STRUCTURED_BUFFER_CONSTANTS, bufferConstants);
}

void RendererDX12::SetGraphicsSRV(uint32_t rootParameterIndex, uint32_t descriptorOffset, ResourceDX12 const& resource, D3D12_RESOURCE_STATES stateAfter, unsigned int firstSubresource, unsigned int numSubresources, const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc)
{
	SetStatesIfChanged();
	m_directCommandList->SetShaderResourceView(rootParameterIndex,descriptorOffset,resource,stateAfter,firstSubresource,numSubresources,srvDesc);
}


