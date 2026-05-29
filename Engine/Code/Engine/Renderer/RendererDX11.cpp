#include "Engine/Renderer/RendererDX11.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Renderer/DefaultShader.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Material.hpp"
#include "Engine/ImGui/ImGuiSystem.hpp"
#include "Engine/Core/StaticMesh.hpp"

#include<vector>
#include "ThirdParty/stb/stb_image.h"


#define WIN32_LEAN_AND_MEAN	
#include <windows.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dCompiler.lib")
#if defined(ENGINE_DEBUG_RENDERER)
#endif
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#if defined(OPAQUE)
#undef OPAQUE
#endif

extern Window* g_window;

RendererDX11::RendererDX11(RendererConfig const& config)
	:Renderer(config)
{
}

void RendererDX11::Startup()
{
	//Direct X 11
//----------------------------------------------------------------------------------------------------------------------
#if defined(ENGINE_DEBUG_RENDERER)
	m_dxgiDebugModule = (void*)::LoadLibraryA("dxgidebug.dll");
	if (m_dxgiDebugModule == nullptr)
	{
		ERROR_AND_DIE("Could not load dxgidebug.dll");
	}

	typedef HRESULT(WINAPI* GetDebugModuleCB)(REFIID, void**);
	((GetDebugModuleCB)::GetProcAddress((HMODULE)m_dxgiDebugModule, "DXGIGetDebugInterface"))
		(__uuidof(IDXGIDebug), &m_dxgiDebug);

	if (m_dxgiDebug == nullptr)
	{
		ERROR_AND_DIE("Could not load debug module");
	}
#endif

	CreateDeviceAndSwapChain();
	GetBackBufferTextureAndCreateRenderTargetView();

	//Compile and create vertex and pixel shader
	DefaultShader shaderDefault;
	m_defaultShader = CreateShader("Default", shaderDefault.m_defaultShaderSource);
	BindShader(m_defaultShader);
	m_defaultScreenCopyShader = CreateShader("Default Screen Copy", shaderDefault.m_defaultShaderDX11FullScreenCopySource);

	//Create Vertex buffer
	m_immediateVBO = CreateVertexBuffer(sizeof(Vertex_PCU), sizeof(Vertex_PCU));

	//Create constant Buffers
	m_perFrameCBO = CreateConstantBuffer(sizeof(PerFrameConstants));
	m_cameraCBO = CreateConstantBuffer(sizeof(CameraConstants));
	m_modelCBO = CreateConstantBuffer(sizeof(ModelConstants));
	m_lightCBO = CreateConstantBuffer(sizeof(LightConstants));
	m_colorAdjustmentCBO = CreateConstantBuffer(sizeof(ColorAdjustmentConstants));

	CreateRasterizerStates();

	//create blend states
	CreateBlendStates();

	//Create default Texture
	Image whitePixelImage(IntVec2(2, 2), Rgba8::WHITE, "White");
	m_defaultTexturesBySlot[k_defaultDiffuseSlot] = CreateTextureFromImage(whitePixelImage);
	Image normalMapImage(IntVec2(2, 2), Rgba8::DEFAULT_NORMAL_MAP, "Normal");
	m_defaultTexturesBySlot[k_defaultNormalsSlot] = CreateTextureFromImage(normalMapImage);
	Image specGlossEmitImage(IntVec2(2,2), Rgba8::DEFAULT_SPEC_GLOSS_EMIT_MAP, "SpecGlossEmit");
	m_defaultTexturesBySlot[k_defaultSpecGlossEmitSlot] = CreateTextureFromImage(specGlossEmitImage);

	for(int i = k_defaultSpecGlossEmitSlot; i < NUM_TEXTURE_SLOTS; ++i)
	{
		m_defaultTexturesBySlot[i] = m_defaultTexturesBySlot[k_defaultDiffuseSlot];
	}

	Image blackImage(IntVec2(2,2), Rgba8::BLACK, "Black");
	Image greyImage(IntVec2(2,2), Rgba8::GREY, "Grey");
	CreateTextureFromImage(blackImage);
	CreateTextureFromImage(greyImage);

	IntVec2 dims = g_window->GetClientDimensions();
	m_renderTarget = CreateRenderTarget(dims);

	for(int i = 0; i < NUM_COPY_RENDER_TARGETS; ++i)
	{
		m_renderTargetCopies[i] = CreateCopyRenderTarget(dims);
	}

	m_currentRenderTargetCopyIndex = 0;

	//Create sampler modes
	CreateSamplerModes();
	//set default sampler modes. Will be changed most likely in game code
	m_desiredSamplerModeBySlot[k_defaultDiffuseSlot] = SamplerMode::POINT_CLAMP;
	m_desiredSamplerModeBySlot[k_defaultNormalsSlot] = SamplerMode::BILINEAR_WRAP;
	m_desiredSamplerModeBySlot[k_defaultSpecGlossEmitSlot] = SamplerMode::BILINEAR_WRAP;

	//Creates Default Mats based on BlendModes;
	CreateDefaultMaterials();

	//Create Depth stencil
	CreateDepthStencil();

	HRESULT hr = m_deviceContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), reinterpret_cast<void**>(&m_userDefinedAnnotations));
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create user defined annotations interface!");
	}

#ifndef RENDERER_DX12
	if(m_config.m_useImGUI)
	{
		ImGuiConfig config;
		config.m_renderer = this;
		config.m_window = Window::s_mainWindow;
		config.m_darkMode = true;
		g_imGuiSystem = new ImGuiSystem(config);
		g_imGuiSystem->Startup();
	}
#endif // !RENDERER_DX12
}

void RendererDX11::BeginFrame()
{
	if(g_imGuiSystem)
	{
		g_imGuiSystem->BeginFrame();
	}

	// --- Unbind everything at the start of the frame ---
	ID3D11RenderTargetView* nullRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
	m_deviceContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, nullRTVs, nullptr);

#ifndef RENDERER_DX12

	ID3D11RenderTargetView* rtv = m_renderTarget.m_color ? m_renderTarget.m_color->m_renderTargetView : nullptr;
	ID3D11DepthStencilView* dsv = m_renderTarget.m_depth ? m_renderTarget.m_depth->m_depthStencilView : nullptr;

	m_deviceContext->OMSetRenderTargets(1, &rtv, dsv);
#endif

	ClearScreen(Rgba8::MAGENTA);
	ClearDepth();

}

void RendererDX11::EndFrame()
{ 

	if (g_imGuiSystem)
		g_imGuiSystem->EndFrame();

	// --- Full unbind of all shader resource views ---
	ID3D11ShaderResourceView* nullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
	m_deviceContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
	m_deviceContext->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
	m_deviceContext->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);

	RenderTarget finalRT = GetCopyOfCurrentRenderTarget();

	if (finalRT.m_color && finalRT.m_depth)
	{
		ID3D11RenderTargetView* nullRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		m_deviceContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, nullRTVs, nullptr);

		m_deviceContext->OMSetRenderTargets(1, &m_backBufferRTV, nullptr);
#ifndef RENDERER_DX12
		DrawFullScreenQuad(finalRT.m_color, finalRT.m_depth, m_defaultScreenCopyShader, DepthMode::DISABLED);
#endif

		ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };
		m_deviceContext->PSSetShaderResources(0, 2, nullSRV);
	}
	else
	{
		m_deviceContext->OMSetRenderTargets(1, &m_backBufferRTV, nullptr);
	}

	for (int i = 0; i < NUM_TEXTURE_SLOTS; ++i)
	{
		m_desiredTexturesBySlot[i] = nullptr;
	}

	HRESULT hr = m_swapChain->Present(0, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		std::string reasonText;
		HRESULT reason = m_device->GetDeviceRemovedReason();
		switch (reason)
		{
		case DXGI_ERROR_DEVICE_HUNG: reasonText = "device hung (bad shader or GPU command)"; break;
		case DXGI_ERROR_DEVICE_RESET: reasonText = "driver reset (power event)"; break;
		case DXGI_ERROR_DEVICE_REMOVED: reasonText = "device removed or reset"; break;
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR: reasonText = "internal driver bug"; break;
		default: reasonText = "unknown"; break;
		}
		ERROR_AND_DIE(Stringf("Device lost: %s", reasonText.c_str()));
	}
}

void RendererDX11::Shutdown()
{
	if(g_imGuiSystem)
	{
		g_imGuiSystem->Shutdown();
		delete g_imGuiSystem;
		g_imGuiSystem = nullptr;
	}
	//release DirectX objects

	DX_SAFE_RELEASE(m_device);

	DX_SAFE_RELEASE(m_deviceContext);
	DX_SAFE_RELEASE(m_backBufferRTV);

	DX_SAFE_RELEASE(m_swapChain);
	DX_SAFE_RELEASE(m_userDefinedAnnotations);
	DX_SAFE_RELEASE(m_depthStencilSRV);
	DX_SAFE_RELEASE(m_depthStencilTexture);
	DX_SAFE_RELEASE(m_backBufferDSV);

	delete(m_immediateVBO);
	m_immediateVBO = nullptr;

	delete(m_cameraCBO);
	m_cameraCBO = nullptr;

	delete(m_modelCBO);
	m_modelCBO = nullptr;

	delete(m_lightCBO);
	m_lightCBO = nullptr;

	delete(m_colorAdjustmentCBO);
	m_colorAdjustmentCBO = nullptr;

	delete(m_perFrameCBO);
	m_perFrameCBO = nullptr;

	for (int shaderNum = 0; shaderNum < (int)m_loadedShaders.size(); ++shaderNum)
	{
		delete(m_loadedShaders[shaderNum]);
		m_loadedShaders[shaderNum] = nullptr;
	}

	for (int blendStateNum = 0; blendStateNum < (int)BlendMode::COUNT; ++blendStateNum)
	{
		DX_SAFE_RELEASE(m_blendStates[blendStateNum]);
	}

	for (int samplerModeNum = 0; samplerModeNum < (int)SamplerMode::COUNT; ++samplerModeNum)
	{
		DX_SAFE_RELEASE(m_samplerStates[samplerModeNum]);
	}

	for (int rasterizerModeNum = 0; rasterizerModeNum < (int)RasterizerMode::COUNT; ++rasterizerModeNum)
	{
		DX_SAFE_RELEASE(m_rasterizerStates[rasterizerModeNum]);
	}

	for (int depthStencilNum = 0; depthStencilNum < (int)DepthMode::COUNT; ++depthStencilNum)
	{
		DX_SAFE_RELEASE(m_depthStencilStates[depthStencilNum]);
	}

	for (int textureNum = 0; textureNum < (int)m_loadedTextures.size(); ++textureNum)
	{
		Texture*& currentTexture = m_loadedTextures[textureNum];
		DX_SAFE_RELEASE(currentTexture->m_texture);
		DX_SAFE_RELEASE(currentTexture->m_shaderResourceView);
		DX_SAFE_RELEASE(currentTexture->m_renderTargetView);
		DX_SAFE_RELEASE(currentTexture->m_depthStencilView);

		delete(currentTexture);
		currentTexture = nullptr;
	}

	for (int i = 0; i < (int)m_loadedStaticMeshes.size(); ++i)
	{
		delete m_loadedStaticMeshes[i];
		m_loadedStaticMeshes[i] = nullptr;
	}


	//report error leaks and release debug module
#if defined(ENGINE_DEBUG_RENDERER)
	((IDXGIDebug*)m_dxgiDebug)->ReportLiveObjects(
		DXGI_DEBUG_ALL,
		(DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)
	);

	((IDXGIDebug*)m_dxgiDebug)->Release();
	m_dxgiDebug = nullptr;

	::FreeLibrary((HMODULE)m_dxgiDebugModule);
	m_dxgiDebugModule = nullptr;
#endif
}

void RendererDX11::ClearScreen(const Rgba8& clearColor)
{
	if(!m_activeRenderTarget)
		return;

	float colorAsFloats[4];
	clearColor.GetAsFloats(colorAsFloats);

#ifndef RENDERER_DX12

	RenderTarget target = m_renderTarget;

	if (target.m_color)
	{
		m_deviceContext->ClearRenderTargetView(target.m_color->m_renderTargetView, colorAsFloats);
	}

	if (target.m_depth)
	{
		m_deviceContext->ClearDepthStencilView(target.m_depth->m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	}
#endif // !RENDERER_DX12
}

void RendererDX11::ClearDepth()
{
	if (!m_activeRenderTarget)
		return;

#ifndef RENDERER_DX12
	if (m_renderTarget.m_depth)
	{
		m_deviceContext->ClearDepthStencilView(m_renderTarget.m_depth->m_depthStencilView,
			D3D11_CLEAR_DEPTH, 1.f, 0);
	}
#endif // !RENDERER_DX12
}

void RendererDX11::BeginCamera(const Camera& camera)
{

	Vec2 screenBottomLeft = camera.GetOrthoBottomLeft();
	Vec2 screenTopRight = camera.GetOrthoTopRight();

	CameraConstants cameraConstants;
	cameraConstants.m_worldToCameraTransform = camera.GetWorldToCameraTransform();
	cameraConstants.m_cameraToRenderTransform = camera.GetCameraToRenderTransform();
	cameraConstants.m_renderToClipTransform = camera.GetRenderToClipTransform();
	cameraConstants.m_cameraPosition = camera.GetPosition();
	CopyCPUToGPU(&cameraConstants, sizeof(CameraConstants), m_cameraCBO);
	BindConstantBuffer(s_cameraConstantsSlot, m_cameraCBO);

	//set model constants to default
	ModelConstants modelConstants;
	modelConstants.m_modelToWorldTransform = Mat44::IDENTITY;
	modelConstants.m_modelColor[0] = 1.f;
	modelConstants.m_modelColor[1] = 1.f;
	modelConstants.m_modelColor[2] = 1.f;
	modelConstants.m_modelColor[3] = 1.f;
	CopyCPUToGPU(&modelConstants, sizeof(modelConstants), m_modelCBO);
	BindConstantBuffer(s_modelConstantsSlot, m_modelCBO);

	ColorAdjustmentConstants colorConstants;
	colorConstants.m_saturation = 1.f;
	SetColorAdjustmentConstants(colorConstants);

	//set viewport
	Vec2 clientDims((float)m_config.m_window->GetClientDimensions().x, (float)m_config.m_window->GetClientDimensions().y);
	AABB2 viewportNormalizedBounds = camera.GetViewportBounds();
	float viewportWidth = (viewportNormalizedBounds.m_maxs.x - viewportNormalizedBounds.m_mins.x) * clientDims.x;
	float viewportHeight = (viewportNormalizedBounds.m_maxs.y - viewportNormalizedBounds.m_mins.y) * clientDims.y;

	float topLeftX = viewportNormalizedBounds.m_mins.x * clientDims.x;
	float topLeftY = (1.f - viewportNormalizedBounds.m_maxs.y) * clientDims.y;

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = topLeftX;
	viewport.TopLeftY = topLeftY;
	viewport.Width = viewportWidth;
	viewport.Height = viewportHeight;
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;

	m_deviceContext->RSSetViewports(1, &viewport);
}

void RendererDX11::EndCamera(const Camera& camera)
{
	UNUSED(camera);
}

void RendererDX11::BeginRendererEvent(char const* eventName) const
{
	int eventNameLength = (int)strlen(eventName) + 1;
	int eventNameWideCharLength = MultiByteToWideChar(CP_UTF8, 0, eventName, eventNameLength, NULL, 0);

	wchar_t* eventNameWideCharStr = new wchar_t[eventNameWideCharLength];
	MultiByteToWideChar(CP_UTF8, 0, eventName, eventNameLength, eventNameWideCharStr, eventNameWideCharLength);

	m_userDefinedAnnotations->BeginEvent(eventNameWideCharStr);
}

void RendererDX11::EndRendererEvent() const
{
	m_userDefinedAnnotations->EndEvent();
}


//Draw
//----------------------------------------------------------------------------------------------------------------------

void RendererDX11::DrawVertexArray(int numVertexes, const Vertex_PCU* vertexes)
{
	unsigned int unsignedNumVertexes = (unsigned int)numVertexes;
	unsigned int size = unsignedNumVertexes * (unsigned int)sizeof(Vertex_PCU);
	CopyCPUToGPU(vertexes, size, m_immediateVBO);
	DrawVertexBuffer(m_immediateVBO, unsignedNumVertexes);
}

void RendererDX11::DrawVertexArray(const std::vector<Vertex_PCU> verts)
{
	DrawVertexArray((int)(verts.size()), verts.data());
}

void RendererDX11::DrawVertexBuffer(VertexBuffer* vbo, unsigned int vertexCount)
{
	BindVertexBuffer(vbo);
	SetStatesIfChanged();
	m_deviceContext->Draw(vertexCount, 0);
}

void RendererDX11::DrawIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int indexedCount)
{
	BindVertexBuffer(vbo);
	BindIndexBuffer(ibo);
	SetStatesIfChanged();
	m_deviceContext->DrawIndexed(indexedCount, 0, 0);
}

//DX change states
//----------------------------------------------------------------------------------------------------------------------

void RendererDX11::SetStatesIfChanged()
{
	if (m_blendStates[(int)m_desiredBlendMode] != m_blendState)
	{
		m_blendState = m_blendStates[(int)m_desiredBlendMode];
		float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
		unsigned int sampleMask = 0xffffffff;
		m_deviceContext->OMSetBlendState(m_blendState, blendFactor, sampleMask);
	}

	for (int slotNum = 0; slotNum < NUM_TEXTURE_SLOTS; ++slotNum)
	{
		if (m_samplerStates[(int)m_desiredSamplerModeBySlot[slotNum]] != m_samplerStateBySlot[slotNum])
		{
			m_samplerStateBySlot[slotNum] = m_samplerStates[(int)m_desiredSamplerModeBySlot[slotNum]];
			m_deviceContext->PSSetSamplers((unsigned int)slotNum, 1, &m_samplerStateBySlot[slotNum]);
		}
	}

	if (m_rasterizerStates[(int)m_desiredRasterizerMode] != m_rasterizerState)
	{
		m_rasterizerState = m_rasterizerStates[(int)m_desiredRasterizerMode];
		m_deviceContext->RSSetState(m_rasterizerState);
	}

	if (m_depthStencilStates[(int)m_desiredDepthMode] != m_depthStencilState)
	{
		m_depthStencilState = m_depthStencilStates[(int)m_desiredDepthMode];
		m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 0);
	}

	if (m_desiredShader != m_currentShader)
	{
		if (m_desiredShader == nullptr)
		{
			m_desiredShader = m_defaultShader;
		}

		m_deviceContext->VSSetShader(m_desiredShader->m_vertexShader, nullptr, 0);
		m_deviceContext->PSSetShader(m_desiredShader->m_pixelShader, nullptr, 0);
		m_deviceContext->IASetInputLayout(m_desiredShader->m_inputLayout);
		m_currentShader = m_desiredShader;

		for (int i = 0; i < NUM_TEXTURE_SLOTS; ++i)
		{
			m_currentTexturesBySlot[i] = nullptr;
		}
	}

	for(int slot = 0; slot < NUM_TEXTURE_SLOTS; ++slot)
	{
		Texture const* texture = m_desiredTexturesBySlot[slot];

		if(texture == m_currentTexturesBySlot[slot])
			continue;

		if (texture == nullptr)
		{
			if (m_defaultTexturesBySlot[slot])
			{
				texture = m_defaultTexturesBySlot[slot];
			}

			else
			{
				texture = m_defaultTexturesBySlot[0];
			}
		}

		m_deviceContext->PSSetShaderResources((unsigned int)slot, 1, &texture->m_shaderResourceView);
		m_currentTexturesBySlot[slot] = texture;
	}
}

void RendererDX11::DrawFullScreenQuad(Texture const* colorTexture, Texture const* depthTexture, Shader const* shader, DepthMode depthMode)
{

	BeginRendererEvent("Draw - Full Screen Quad");
	Vec2 screenMax(m_config.m_window->GetClientDimensions());
	AABB2 fullScreenQuad = AABB2(Vec2::ONE * -1.f, Vec2::ONE);
	IntVec2 clientDims = m_config.m_window->GetClientDimensions();

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.f;
	viewport.TopLeftY = 0.f;
	viewport.Width = (float)clientDims.x;
	viewport.Height = (float)clientDims.y;
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;

	m_deviceContext->RSSetViewports(1, &viewport);

	Verts fullScreenVerts;
	AddVertsForAABB2D(fullScreenVerts, fullScreenQuad, Rgba8::WHITE, AABB2(0.f, 1.f, 1.f, 0.f));
	SetSamplerMode(SamplerMode::POINT_CLAMP);
	SetDepthMode(depthMode);
	SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	SetBlendMode(BlendMode::OPAQUE);
	BindTexture(colorTexture);
	BindTexture(depthTexture, 1);

	if(shader)
		BindShader(shader);
	else
		BindShader(m_defaultScreenCopyShader);

	DrawVertexArray(fullScreenVerts);
	EndRendererEvent();
}


//Textures
//----------------------------------------------------------------------------------------------------------------------
BitmapFont* RendererDX11::CreateOrGetBitMapFontFromFile(char const* bitmapFontFilePathWithNoExtension)
{
	BitmapFont* existingBitMapFont = GetBitMapFontForFileName(bitmapFontFilePathWithNoExtension);
	if (existingBitMapFont)
	{
		return existingBitMapFont;
	}

	BitmapFont* newBitMapFont = nullptr;
	//#TODO: need to remove the adding file extension line
	std::string textureFilePath = Stringf("%s.png", bitmapFontFilePathWithNoExtension);
#ifndef RENDERER_DX12

	Texture * newTexture = CreateOrGetTextureFromFile(textureFilePath.c_str());
	newBitMapFont = new BitmapFont(bitmapFontFilePathWithNoExtension, *newTexture, IntVec2(16, 16));
	m_loadedFonts.push_back(newBitMapFont);

#endif 

	return newBitMapFont;
}


Texture* RendererDX11::CreateOrGetTextureFromFile(char const* imageFilePath)
{
	// See if we already have this texture previously loaded
	Texture* existingTexture = GetTextureForFileName(imageFilePath);
	if (existingTexture)
	{
		return existingTexture;
	}

	// Never seen this texture before!  Let's load it.
	Texture* newTexture = CreateTextureFromFile(imageFilePath);
	return newTexture;
}

Texture* RendererDX11::CreateOrGetMipMapTextureFromFile(char const* imageFilePath, unsigned int numMipLevels)
{
	// 1. Check cache
	Texture* existingTexture = GetTextureForFileName(imageFilePath);
	if (existingTexture)
	{
		D3D11_TEXTURE2D_DESC desc;
		existingTexture->m_texture->GetDesc(&desc);

		const bool hasMips = desc.MipLevels > 1;
		const bool canGenerateMips = (desc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS);
		const bool sameMipCount = (numMipLevels == 0 && canGenerateMips) || (numMipLevels == desc.MipLevels);

		if (hasMips && sameMipCount)
			return existingTexture;

		return CreateMipMappedCopy(existingTexture, numMipLevels);
	}

	// 2. Not found — load new texture with mips
	Image image(imageFilePath);
	return CreateTextureWithMipMaps(image, numMipLevels);
}

Texture* RendererDX11::GetTextureForFileName(char const* imageFilePath) const
{
	for (int textureIndex = 0; textureIndex < (int)(m_loadedTextures.size()); ++textureIndex)
	{
		if (m_loadedTextures[textureIndex]->m_name == static_cast<std::string>(imageFilePath))
		{
			return m_loadedTextures[textureIndex];
		}
	}

	return nullptr;
}

Texture* RendererDX11::CreateTextureFromFile(char const* imageFilePath)
{
	Image fileImage(imageFilePath);
	return CreateTextureFromImage(fileImage);
}

Texture* RendererDX11::CreateTextureFromImage(Image const& image)
{
	Texture* newTexture = new Texture();
	newTexture->m_dimensions = image.GetDimensions();
	newTexture->m_name = image.GetImageFilePath();


	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = image.GetDimensions().x;
	textureDesc.Height = image.GetDimensions().y;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA textureData;
	textureData.pSysMem = image.GetRawData();
	textureData.SysMemPitch = 4 * image.GetDimensions().x;

	HRESULT hr = m_device->CreateTexture2D(&textureDesc, &textureData, &newTexture->m_texture);

	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("CreateTextureFromImage failed for image file: \"%s\"", image.GetImageFilePath().c_str()));
	}

	hr = m_device->CreateShaderResourceView(newTexture->m_texture, NULL, &newTexture->m_shaderResourceView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("CreateShaderResourceView failed for image file: \"%s\"", image.GetImageFilePath().c_str()));
	}

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

Texture* RendererDX11::CreateRenderTargetCopyColorTexture(IntVec2 const& dimensions)
{
	Texture* newTexture = new Texture();
	newTexture->m_dimensions = dimensions;
	newTexture->m_name = "RenderTargetCopy_ColorTexture";


	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = dimensions.x;
	textureDesc.Height = dimensions.y;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	HRESULT hr = m_device->CreateTexture2D(&textureDesc, nullptr, &newTexture->m_texture);

	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Failed to create render-target copy texture.");
	}

	hr = m_device->CreateShaderResourceView(newTexture->m_texture, NULL, &newTexture->m_shaderResourceView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Failed to create SRV for render-target copy texture.");
	}

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

Texture* RendererDX11::CreateRenderTargetCopyDepthTexture(IntVec2 const& dimensions)
{
	Texture* depthTex = new Texture();
	depthTex->m_name = "RenderTargetCopy_DepthTexture";
	depthTex->m_dimensions = dimensions;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = dimensions.x;
	desc.Height = dimensions.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &depthTex->m_texture);
	if (!SUCCEEDED(hr))
	{
		delete depthTex;
		ERROR_AND_DIE("Failed to create depth copy texture.");
	}

	// Create shader resource view matching the master SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;  // View format for depth
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	hr = m_device->CreateShaderResourceView(depthTex->m_texture, &srvDesc, &depthTex->m_shaderResourceView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Failed to create SRV for depth copy texture.");
	}

	m_loadedTextures.push_back(depthTex);
	return depthTex;
}

Texture* RendererDX11::CreateMipMappedCopy(Texture* texture, int numMipLevels)
{
	D3D11_TEXTURE2D_DESC srcDesc;
	texture->m_texture->GetDesc(&srcDesc);

	D3D11_TEXTURE2D_DESC texDesc = srcDesc;
	texDesc.MipLevels = numMipLevels;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	Texture* newTexture = new Texture();
	newTexture->m_name = texture->m_name;
	newTexture->m_dimensions = texture->m_dimensions;

	HRESULT hr = m_device->CreateTexture2D(&texDesc, nullptr, &newTexture->m_texture);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("Failed to create mipmapped copy (mipLevels=%u) for: \"%s\"", numMipLevels, newTexture->m_name.c_str()));
	}

	// SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = (numMipLevels == 0) ? -1 : numMipLevels;

	hr = m_device->CreateShaderResourceView(newTexture->m_texture, &srvDesc, &newTexture->m_shaderResourceView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("Failed to create SRV (mipmaps=%u) for: \"%s\"", numMipLevels, newTexture->m_name.c_str()));
	}

	// Copy base
	m_deviceContext->CopySubresourceRegion(newTexture->m_texture, 0, 0, 0, 0, texture->m_texture, 0, nullptr);

	// Generate chain
	if (numMipLevels != 1)
		m_deviceContext->GenerateMips(newTexture->m_shaderResourceView);

	newTexture->m_numMipLevels = numMipLevels;
	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

Texture* RendererDX11::CreateTextureWithMipMaps(Image const& image, int numMipLevels)
{
	if ((image.GetDimensions().x <= 0) || (image.GetDimensions().y <= 0))
	{
		ERROR_AND_DIE(Stringf("Invalid texture size (%dx%d) for: \"%s\"",
			image.GetDimensions().x, image.GetDimensions().y, image.GetImageFilePath().c_str()));
	}

	Texture* newTexture = new Texture();
	newTexture->m_dimensions = image.GetDimensions();
	newTexture->m_name = image.GetImageFilePath();

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = image.GetDimensions().x;
	textureDesc.Height = image.GetDimensions().y;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MipLevels = numMipLevels;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	HRESULT hr = m_device->CreateTexture2D(&textureDesc, nullptr, &newTexture->m_texture);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("Failed to create texture (mipmaps=%u) for image: \"%s\"", numMipLevels, image.GetImageFilePath().c_str()));
	}

	// SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = (numMipLevels == 0) ? -1 : numMipLevels;

	hr = m_device->CreateShaderResourceView(newTexture->m_texture, &srvDesc, &newTexture->m_shaderResourceView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("Failed to create SRV (mipmaps=%u) for image: \"%s\"", numMipLevels, image.GetImageFilePath().c_str()));
	}

	// Upload base level
	m_deviceContext->UpdateSubresource(newTexture->m_texture, 0, nullptr, image.GetRawData(), 4 * image.GetDimensions().x, 0);

	// Generate if needed
	if (numMipLevels != 1)
		m_deviceContext->GenerateMips(newTexture->m_shaderResourceView);
	

	D3D11_TEXTURE2D_DESC desc;
	newTexture->m_texture->GetDesc(&desc);
	newTexture->m_numMipLevels = desc.MipLevels;

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

void RendererDX11::BindTexture(Texture const* texture, int slot)
{
	if (texture)
	{
		m_desiredTexturesBySlot[slot] = texture;
#ifndef RENDERER_DX12
		if (texture->m_texture == m_renderTarget.m_color->m_texture || texture->m_texture == m_renderTarget.m_depth->m_texture)
		{
			ERROR_AND_DIE("Trying to bind an active render target");
		}
#endif
	}

	else
	{
		m_desiredTexturesBySlot[slot] = m_defaultTexturesBySlot[slot];
	}
}


//Shaders
//----------------------------------------------------------------------------------------------------------------------

Shader* RendererDX11::CreateOrGetShaderFromFile(char const* shaderName, VertexType vertexType)
{
	std::string shaderString = (std::string)shaderName;
	shaderString.append(".hlsl");

	for (int shaderIndex = 0; shaderIndex < (int)m_loadedShaders.size(); ++shaderIndex)
	{
		if (m_loadedShaders[shaderIndex]->GetName() == (std::string)shaderName)
		{
			return m_loadedShaders[shaderIndex];
		}
	}
	std::string shaderSource;
	FileReadToString(shaderSource, shaderString);
	return CreateShader(shaderName, shaderSource.c_str(), vertexType);
}

Shader* RendererDX11::CreateShader(char const* shaderName, char const* shaderSource, VertexType vertexType)
{
	ShaderConfig shaderConfig;
	shaderConfig.m_name = shaderName;

	Shader* newShader = new Shader(shaderConfig);

	//vertex shader
	std::vector<unsigned char> vertexShaderByteCode;

	if (!CompileShaderToByteCode(vertexShaderByteCode, shaderName, shaderSource, shaderConfig.m_vertexEntryPoint.c_str(), "vs_5_0"))
	{
		ERROR_AND_DIE(Stringf("could not compile vertex shader for: \"%s\"", shaderName));
	}

	//create vertex shader
	HRESULT hr;
	hr = m_device->CreateVertexShader(
		vertexShaderByteCode.data(),
		vertexShaderByteCode.size(),
		NULL, &newShader->m_vertexShader
	);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("Could not create vertex shader for: \"%s\"", shaderName));
	}

	//pixel shader
	std::vector<unsigned char> pixelShaderByteCode;
	if (!CompileShaderToByteCode(pixelShaderByteCode, shaderName, shaderSource, shaderConfig.m_pixelEntryPoint.c_str(), "ps_5_0"))
	{
		ERROR_AND_DIE(Stringf("could not compile pixel shader for: \"%s\" ", shaderName));
	}

	//create pixel shader
	hr = m_device->CreatePixelShader(
		pixelShaderByteCode.data(),
		pixelShaderByteCode.size(),
		NULL, &newShader->m_pixelShader);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("Could not create pixel shader for: \"%s\"", shaderName));
	}

	//input element descriptions to define vertex input layout
	D3D11_INPUT_ELEMENT_DESC inputElementDescVerts_PCU[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
		0,0, D3D11_INPUT_PER_VERTEX_DATA,0},
		{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM,
		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT,
		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
	};

	D3D11_INPUT_ELEMENT_DESC inputElementDescVerts_PCUTBN[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
		0,0, D3D11_INPUT_PER_VERTEX_DATA,0},
		{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM,
		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT,
		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,
		0,D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,
		0,D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
		0,D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},

	};

	unsigned int numElements;
	switch (vertexType)
	{
	case VertexType::VERTEX_PCU:
		numElements = ARRAYSIZE(inputElementDescVerts_PCU);
		hr = m_device->CreateInputLayout(
			inputElementDescVerts_PCU, numElements,
			vertexShaderByteCode.data(),
			vertexShaderByteCode.size(),
			&newShader->m_inputLayout
		);
		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE("Could not create vertex layout for Vertex_PCU");
		}
		break;

	case VertexType::VERTEX_PCUTBN:
		numElements = ARRAYSIZE(inputElementDescVerts_PCUTBN);
		hr = m_device->CreateInputLayout(
			inputElementDescVerts_PCUTBN, numElements,
			vertexShaderByteCode.data(),
			vertexShaderByteCode.size(),
			&newShader->m_inputLayout
		);
		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE("Could not create vertex layout for Vertex_PCUTBN");
		}
		break;

	default:
		ERROR_AND_DIE("Vertex type was not set to supported type");
		break;
	}

	m_loadedShaders.push_back(newShader);
	return newShader;
}

Shader* RendererDX11::CreateShader(char const* shaderName, VertexType vertexType)
{
	//Create data path
	std::string shaderSource = Stringf("Data/Shaders/%s.hlsl", shaderName);

	std::string outString;
	FileReadToString(outString, shaderSource);
	return CreateShader(shaderName, outString.c_str(), vertexType);
}

void RendererDX11::BindShader(Shader const* shader)
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

//Buffers
//----------------------------------------------------------------------------------------------------------------------
void RendererDX11::CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vbo)
{
	if (vbo->m_size < size)
	{
		vbo->Resize(size);
	}

	//copy data from cpu to gpu
	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map(vbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(vbo->m_buffer, 0);
}

void RendererDX11::CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* cbo)
{
	//copy data from cpu to gpu
	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map(cbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(cbo->m_buffer, 0);
}

void RendererDX11::CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* ibo)
{
	if (ibo->m_size < size)
	{
		ibo->Resize(size);
	}

	//copy data from cpu to gpu
	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map(ibo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(ibo->m_buffer, 0);
}

VertexBuffer* RendererDX11::CreateVertexBuffer(const unsigned int size, unsigned int stride)
{
	return new VertexBuffer(m_device, size, stride);
}

void RendererDX11::BindVertexBuffer(VertexBuffer* vbo)
{
	unsigned int startOffset = 0;
	m_deviceContext->IASetVertexBuffers(0, 1, &vbo->m_buffer, &vbo->m_stride, &startOffset);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

ConstantBuffer* RendererDX11::CreateConstantBuffer(const unsigned int size)
{
	return new ConstantBuffer(m_device, size);
}
void RendererDX11::BindConstantBuffer(int slot, ConstantBuffer* cbo)
{
	m_deviceContext->VSSetConstantBuffers((unsigned int)slot, 1, &cbo->m_buffer);
	m_deviceContext->PSSetConstantBuffers((unsigned int)slot, 1, &cbo->m_buffer);
}


IndexBuffer* RendererDX11::CreateIndexBuffer(const unsigned int size)
{
	return new IndexBuffer(m_device, size);
}


void RendererDX11::BindIndexBuffer(IndexBuffer* ibo)
{
	unsigned int startOffset = 0;
	m_deviceContext->IASetIndexBuffer(ibo->m_buffer, DXGI_FORMAT_R32_UINT, startOffset);
}

void RendererDX11::SetModelConstants(Mat44 const& modelToWorldTransform, Rgba8 const& modelColor)
{
	ModelConstants modelConstant;
	modelConstant.m_modelToWorldTransform = modelToWorldTransform;
	float color[4];
	modelColor.GetAsFloats(color);
	modelConstant.m_modelColor[0] = color[0];
	modelConstant.m_modelColor[1] = color[1];
	modelConstant.m_modelColor[2] = color[2];
	modelConstant.m_modelColor[3] = color[3];
	CopyCPUToGPU(&modelConstant, sizeof(ModelConstants), m_modelCBO);
	BindConstantBuffer(s_modelConstantsSlot, m_modelCBO);
}

void RendererDX11::SetLightConstants(Vec3 const& sunDirection, float sunIntensity, float ambientIntensity, Rgba8 const& sunColor)
{
	LightConstants lightConstant;
	lightConstant.m_sunDirection = sunDirection;
	lightConstant.m_sunIntensity = sunIntensity;
	lightConstant.m_ambientIntensity = ambientIntensity;
	lightConstant.m_sunColorRGB[0] = NormalizeByte(sunColor.r);
	lightConstant.m_sunColorRGB[1] = NormalizeByte(sunColor.g);
	lightConstant.m_sunColorRGB[2] = NormalizeByte(sunColor.b);
	CopyCPUToGPU(&lightConstant, sizeof(LightConstants), m_lightCBO);
	BindConstantBuffer(s_lightConstantsSlot, m_lightCBO);
}

void RendererDX11::SetLightConstants(LightConstants const& lightConstants)
{
	CopyCPUToGPU(&lightConstants, sizeof(LightConstants), m_lightCBO);
	BindConstantBuffer(s_lightConstantsSlot, m_lightCBO);
}

void RendererDX11::SetColorAdjustmentConstants(ColorAdjustmentConstants const& colorAdjustmentConstants)
{
	CopyCPUToGPU(&colorAdjustmentConstants, sizeof(ColorAdjustmentConstants), m_colorAdjustmentCBO);
	BindConstantBuffer(s_colorAdjustmentConstantsSlot, m_colorAdjustmentCBO);
}

void RendererDX11::SetPerFrameConstants(PerFrameConstants const& perFrameConstants)
{
	CopyCPUToGPU(&perFrameConstants, sizeof(PerFrameConstants), m_perFrameCBO);
	BindConstantBuffer(s_perFrameConstantsSlot, m_perFrameCBO);
}

Material* RendererDX11::GetDefaultMaterialByBlendMode(BlendMode blendMode)
{
	return m_defaultMats[(int)blendMode];
}

//DX set up
//----------------------------------------------------------------------------------------------------------------------

void RendererDX11::CreateDeviceAndSwapChain()
{
	unsigned int deviceFlags = 0;
#if defined(ENGINE_DEBUG_RENDERER)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	//Create device and swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width =  g_window->GetClientDimensions().x;
	swapChainDesc.BufferDesc.Height = g_window->GetClientDimensions().y;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = (HWND)g_window->GetHwnd();
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;


	HRESULT hr;
	hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags,
		nullptr, 0, D3D11_SDK_VERSION, &swapChainDesc,
		&m_swapChain, &m_device, nullptr, &m_deviceContext);

	std::string debugName = Stringf("m_device");
	m_device->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(debugName.c_str())), debugName.c_str());

	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create D3D 11 device and swap chain.");
	}
}

void RendererDX11::GetBackBufferTextureAndCreateRenderTargetView()
{
	//get back buffer texture
	ID3D11Texture2D* backBuffer;
	HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not get chain swap buffer.")
	}

	//create render target view
	hr = m_device->CreateRenderTargetView(backBuffer, NULL, &m_backBufferRTV);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create render target view for swap chain buffer.")
	}

	DX_SAFE_RELEASE(backBuffer);
}

RenderTarget RendererDX11::CreateRenderTarget(IntVec2 const& dimensions)
{
	RenderTarget renderTarget;
#ifndef RENDERER_DX12
	renderTarget.m_color = CreateRenderTexture(dimensions);
	renderTarget.m_depth = CreateDepthTexture(dimensions);
#else
	UNUSED(dimensions);

#endif // !RENDERER_DX12
	return renderTarget;
}

Texture* RendererDX11::CreateRenderTexture(IntVec2 const& dimensions)
{
	Texture* renderTex = new Texture();
	renderTex->m_name = "RenderTexture";
	renderTex->m_dimensions = dimensions;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = dimensions.x;
	desc.Height = dimensions.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &renderTex->m_texture);
	if (!SUCCEEDED(hr))
	{
		delete renderTex;
		ERROR_AND_DIE("Failed to create render texture.");
	}

	// Create RTV
	hr = m_device->CreateRenderTargetView(renderTex->m_texture, nullptr, &renderTex->m_renderTargetView);
	if (!SUCCEEDED(hr))
	{
		delete renderTex;
		ERROR_AND_DIE("Failed to create RTV for render texture.");
	}

	// Create SRV
	hr = m_device->CreateShaderResourceView(renderTex->m_texture, nullptr, &renderTex->m_shaderResourceView);
	if (!SUCCEEDED(hr))
	{
		delete renderTex;
		ERROR_AND_DIE("Failed to create SRV for render texture.");
	}

	m_loadedTextures.push_back(renderTex);

	return renderTex;
}

Texture* RendererDX11::CreateDepthTexture(IntVec2 const& dimensions)
{
	Texture* depthTex = new Texture();
	depthTex->m_name = "DepthTexture";
	depthTex->m_dimensions = dimensions;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = dimensions.x;
	desc.Height = dimensions.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &depthTex->m_texture);
	if (!SUCCEEDED(hr))
	{
		delete depthTex;
		ERROR_AND_DIE("Failed to create depth texture.");
	}

	// DSV
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	hr = m_device->CreateDepthStencilView(depthTex->m_texture, &dsvDesc, &depthTex->m_depthStencilView);
	if (!SUCCEEDED(hr))
	{
		delete depthTex;
		ERROR_AND_DIE("Failed to create DSV for depth texture.");
	}

	// SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	hr = m_device->CreateShaderResourceView(depthTex->m_texture, &srvDesc, &depthTex->m_shaderResourceView);
	if (!SUCCEEDED(hr))
	{
		delete depthTex;
		ERROR_AND_DIE("Failed to create SRV for depth texture.");
	}
	m_loadedTextures.push_back(depthTex);
	return depthTex;
}

RenderTarget RendererDX11::CreateCopyRenderTarget(IntVec2 const& dimensions)
{
	RenderTarget newCopyRT;
#ifndef RENDERER_DX12
	newCopyRT.m_color = CreateRenderTargetCopyColorTexture(dimensions);
	newCopyRT.m_depth = CreateRenderTargetCopyDepthTexture(dimensions);
#else
	UNUSED(dimensions);
#endif
	return newCopyRT;
}

void RendererDX11::CreateRasterizerStates()
{
	//Solid cull none mode
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.f;
	rasterizerDesc.SlopeScaledDepthBias = 0.f;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.ScissorEnable = false;
	rasterizerDesc.MultisampleEnable = false;
	rasterizerDesc.AntialiasedLineEnable = true;

	HRESULT hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[(int)RasterizerMode::SOLID_CULL_NONE]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create rasterizer state for SOLID_CULL_NONE");
	}

	//Solid cull back
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;

	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[(int)RasterizerMode::SOLID_CULL_BACK]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create rasterizer state for SOLID_CULL_BACK");
	}

	//Solid cull back
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;

	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[(int)RasterizerMode::SOLID_CULL_FRONT]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create rasterizer state for SOLID_CULL_FRONT");
	}

	//WireFrame Cull None
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;

	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[(int)RasterizerMode::WIREFRAME_CULL_NONE]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create rasterizer state for WIRE_FRAME_CULL_NONE");
	}

	//WireFrame Cull Back
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;

	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[(int)RasterizerMode::WIREFRAME_CULL_BACK]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create rasterizer state for WIRE_FRAME_CULL_BACK");
	}

	//WireFrame Cull Front
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;

	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[(int)RasterizerMode::WIREFRAME_CULL_FRONT]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create rasterizer state for WIRE_FRAME_CULL_FRONT");
	}
}

void RendererDX11::CreateBlendStates()
{
	//Opaque
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = blendDesc.RenderTarget[0].SrcBlend;
	blendDesc.RenderTarget[0].DestBlendAlpha = blendDesc.RenderTarget[0].DestBlend;
	blendDesc.RenderTarget[0].BlendOpAlpha = blendDesc.RenderTarget[0].BlendOp;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HRESULT hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::OPAQUE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::OPAQUE failed");
	}

	//Alpha
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::ALPHA)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::ALPHA failed");
	}

	//Additive
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::ADDITIVE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::ADDITIVE failed");
	}



	//Lighten
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::LIGHTEN)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::LIGHTEN failed");
	}
}

void RendererDX11::CreateSamplerModes()
{
	//Point Clamp
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HRESULT hr = m_device->CreateSamplerState(&samplerDesc, &m_samplerStates[(int)SamplerMode::POINT_CLAMP]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateSamplerState for SamplerMode::POINT_CLAMP failed.");
	}

	//Bilinear wrap
	samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = m_device->CreateSamplerState(&samplerDesc, &m_samplerStates[(int)SamplerMode::BILINEAR_WRAP]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateSamplerState for SamplerMode::BILINEAR_WRAP failed.");
	}

	//Anisotropic wrap
	samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	samplerDesc.MaxAnisotropy = 8; 
	hr = m_device->CreateSamplerState(&samplerDesc, &m_samplerStates[(int)SamplerMode::ANISOTROPIC_WRAP]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateSamplerState for SamplerMode::ANISOTROPIC_WRAP failed.");
	}

	m_deviceContext->PSSetSamplers(0, 1, &m_samplerStates[0]);
	m_deviceContext->PSSetSamplers(1, 1, &m_samplerStates[1]);
	m_deviceContext->PSSetSamplers(2, 1, &m_samplerStates[2]);

}

void RendererDX11::CreateDepthStencil()
{
	IntVec2 dims = m_config.m_window->GetClientDimensions();

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = dims.x;
	desc.Height = dims.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	desc.Format = DXGI_FORMAT_R24G8_TYPELESS;  // typeless so we can make SRV + DSV views

	HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &m_depthStencilTexture);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create typeless depth texture");

	// --- DSV view (for writing depth) ---
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	hr = m_device->CreateDepthStencilView(m_depthStencilTexture, &dsvDesc, &m_backBufferDSV);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create DSV");

	// --- SRV view (for reading depth later) ---
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {}; 
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	hr = m_device->CreateShaderResourceView(m_depthStencilTexture, &srvDesc, &m_depthStencilSRV);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create depth SRV");

	// --- Depth state objects ---
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};

	// Disabled
	hr = m_device->CreateDepthStencilState(&dsDesc, &m_depthStencilStates[(int)DepthMode::DISABLED]);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Disabled depth state failed");

	// Read-only always
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	hr = m_device->CreateDepthStencilState(&dsDesc, &m_depthStencilStates[(int)DepthMode::READ_ONLY_ALWAYS]);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Read-only always state failed");

	// Read-only less-equal
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_device->CreateDepthStencilState(&dsDesc, &m_depthStencilStates[(int)DepthMode::READ_ONLY_LESS_EQUAL]);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Read-only less-equal state failed");

	// Read/write less-equal
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	hr = m_device->CreateDepthStencilState(&dsDesc, &m_depthStencilStates[(int)DepthMode::READ_WRITE_LESS_EQUAL]);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Read-write less-equal state failed");
/*
	D3D11_TEXTURE2D_DESC depthTextureDesc = {};
	depthTextureDesc.Width = m_config.m_window->GetClientDimensions().x;
	depthTextureDesc.Height = m_config.m_window->GetClientDimensions().y;
	depthTextureDesc.MipLevels = 1;
	depthTextureDesc.ArraySize = 1;
	depthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthTextureDesc.SampleDesc.Count = 1;

	HRESULT hr = m_device->CreateTexture2D(&depthTextureDesc, nullptr, &m_depthStencilTexture);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create texture for depth");
	}

	hr = m_device->CreateDepthStencilView(m_depthStencilTexture, nullptr, &m_backBufferDSV);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Clould not create depth stencil view");
	}

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::DISABLED]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::DISABLED failed");
	}

	depthStencilDesc.DepthEnable = TRUE;

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::READ_ONLY_ALWAYS]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::READ_ONLY_ALWAYS failed");
	}

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::READ_ONLY_LESS_EQUAL]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::READ_ONLY_LESS_EQUAL failed");
	}

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::READ_WRITE_LESS_EQUAL]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::READ_WRITE_LESS_EQUAL failed");
	}
*/
}

void RendererDX11::CreateDefaultMaterials()
{
#ifndef RENDERER_DX12
	for (int i = 0; i < (int)BlendMode::COUNT; ++i)
	{
		std::string blendName = GetNameForBlendMode((BlendMode)i);
		m_defaultMats[i] = new Material(this, Stringf("DefaultMat_%s", blendName.c_str()).c_str(), m_defaultShader);
		m_defaultMats[i]->SetBlendMode((BlendMode)i);
	}
#endif
}

RenderTarget RendererDX11::GetCopyOfCurrentRenderTarget()
{
	RenderTarget copyRT = m_renderTargetCopies[m_currentRenderTargetCopyIndex];
	m_currentRenderTargetCopyIndex = (m_currentRenderTargetCopyIndex + 1) % NUM_COPY_RENDER_TARGETS;
	BeginRendererEvent("Copy - Render Target");

#ifndef RENDERER_DX12
	// 1. UNBIND MASTER RT COMPLETELY
	ID3D11RenderTargetView* nullRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
	m_deviceContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, nullRTVs, nullptr);

	// 2. UNBIND SRVs TO PREVENT HAZARDS
	ID3D11ShaderResourceView* nullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
	m_deviceContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
	m_deviceContext->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
	m_deviceContext->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);

	// 3. COPY
	m_deviceContext->CopyResource(copyRT.m_color->m_texture, m_renderTarget.m_color->m_texture);
	m_deviceContext->CopyResource(copyRT.m_depth->m_texture, m_renderTarget.m_depth->m_texture);

	// 4. REBIND MASTER RT FOR CONTINUED DRAWING
	ID3D11RenderTargetView* rtv = m_renderTarget.m_color->m_renderTargetView;
	ID3D11DepthStencilView* dsv = m_renderTarget.m_depth->m_depthStencilView;
	m_deviceContext->OMSetRenderTargets(1, &rtv, dsv);
#endif
	EndRendererEvent();
	return copyRT;
}

bool RendererDX11::CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, char const* name, char const* source, char const* entryPoint, char const* target)
{
	//compile vertex shader
	DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(ENGINE_DEBUG_RENDERER)
	shaderFlags = D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif
	ID3DBlob* shaderBlob = NULL;
	ID3DBlob* errorBlob = NULL;

	HRESULT hr;
	hr = D3DCompile(source, strlen(source),name, nullptr, nullptr,entryPoint, target, shaderFlags,0, &shaderBlob, &errorBlob);
	if (SUCCEEDED(hr))
	{
		outByteCode.resize(shaderBlob->GetBufferSize());
		memcpy(
			outByteCode.data(),
			shaderBlob->GetBufferPointer(),
			shaderBlob->GetBufferSize()
		);
	}
	else
	{
		if (errorBlob != NULL)
		{
			DebuggerPrintf((char*)errorBlob->GetBufferPointer());
		}
		return false;
	}

	DX_SAFE_RELEASE(shaderBlob);

	if (errorBlob != NULL)
	{
		DX_SAFE_RELEASE(errorBlob);
	}

	return true;
}

