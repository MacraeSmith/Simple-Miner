#include "Engine/ImGui/ImGuiSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Window/Window.hpp"

#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/backends/imgui_impl_win32.h"

#ifdef RENDERER_DX12
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Renderer/CommandQueue.hpp"
#include "Engine/Renderer/CommandList.hpp"
#include "ThirdParty/imgui/backends/imgui_impl_dx12.h"
#include <dxgi1_4.h>
#else
#include "Engine/Renderer/RendererDX11.hpp"
#include "ThirdParty/imgui/backends/imgui_impl_dx11.h"
#endif // RENDERER_DX12

ImGuiSystem* g_imGuiSystem = nullptr;

ImGuiSystem::ImGuiSystem(ImGuiConfig config)
	:m_config(config)
{
	Strings argStrings;
	argStrings.push_back("True");
	argStrings.push_back("False");
	SubscribeEventCallbackFunction("ShowImGui", argStrings, Event_ShowImGui);
}

ImGuiSystem::~ImGuiSystem()
{
	UnsubscribeEventCallbackFunction("ShowImGui", Event_ShowImGui);
}

void ImGuiSystem::Startup()
{
	ImGui_ImplWin32_EnableDpiAwareness();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	//Style
	if(m_config.m_darkMode)
		ImGui::StyleColorsDark();
	else
		ImGui::StyleColorsLight();

	ImGuiStyle& style = ImGui::GetStyle();
	io.ConfigDpiScaleFonts = true;
	io.ConfigDpiScaleViewports = true;

	// When viewports are enabled, tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	
	ImGui_ImplWin32_Init(m_config.m_window->GetHwnd());

#ifdef RENDERER_DX12
	{
	ID3D12Device* device = m_config.m_renderer->GetDevice();

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = device;
	CommandQueue* commandQueue = m_config.m_renderer->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	init_info.CommandQueue = commandQueue->GetD3D12CommandQueue();
	init_info.NumFramesInFlight = NUM_BUFFER_FRAMES;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;

	//Create dedicated srv descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC imguiHeapDesc = {};
	imguiHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	imguiHeapDesc.NumDescriptors = 256; // plenty for font + UI textures
	imguiHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	imguiHeapDesc.NodeMask = 0;

	HRESULT hr = device->CreateDescriptorHeap(&imguiHeapDesc, IID_PPV_ARGS(&m_imguiSRVHeap));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create dedicated ImGui SRV Heap");
	SetDescriptorHeapName(m_imguiSRVHeap, "ImGui_SRV_Desc_Heap");
	m_heapAllocator.Create(device, m_imguiSRVHeap);


	init_info.SrvDescriptorHeap = m_imguiSRVHeap;
	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_imGuiSystem->m_heapAllocator.Alloc(out_cpu_handle, out_gpu_handle); };
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_imGuiSystem->m_heapAllocator.Free(cpu_handle, gpu_handle); };

	ImGui_ImplDX12_Init(&init_info);
	}

#else
	//#TODO: Add DX11 support

	ImGui_ImplDX11_Init(m_config.m_renderer->GetDevice(), m_config.m_renderer->GetDeviceContext());


#endif // 

}

void ImGuiSystem::Shutdown()
{
#ifdef RENDERER_DX12
	DX_SAFE_RELEASE(m_imguiSRVHeap);
	ImGui_ImplDX12_Shutdown();
#else
	ImGui_ImplDX11_Shutdown();
#endif // RENDERER_DX12

	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiSystem::BeginFrame()
{
	if(!m_isVisible)
		return;

#ifdef RENDERER_DX12
	ImGui_ImplDX12_NewFrame();
#else
	ImGui_ImplDX11_NewFrame();
#endif // RENDERER_DX12
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiSystem::EndFrame()
{
	m_config.m_renderer->BeginRendererEvent("ImGUI");
	ImGuiIO& io = ImGui::GetIO();

	if (m_isVisible)
	{
		if (m_showDemoWindow)
			ImGui::ShowDemoWindow(&m_showDemoWindow);

		ImGui::Render();

#ifdef RENDERER_DX12
		CommandList* commandList = m_config.m_renderer->GetCurrentCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
		ID3D12GraphicsCommandList* graphicsCommandList = commandList->GetGraphicsCommandList();
		graphicsCommandList->SetDescriptorHeaps(1, &m_imguiSRVHeap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), graphicsCommandList);

#else
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif // RENDERER_DX12


		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}
	
	m_config.m_renderer->EndRendererEvent();

}


bool ImGuiSystem::Event_ShowImGui(EventArgs& args)
{
	if (g_imGuiSystem)
	{
		if (args.HasKey("false"))
		{
			g_imGuiSystem->m_isVisible = false;
		}

		else
		{
			g_imGuiSystem->m_isVisible = true;
		}

		return true;
	}
	return false;
}


#ifdef RENDERER_DX12

void ImGUIHeapAllocator::Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
{
	IM_ASSERT(m_heap == nullptr && m_freeIndices.empty());
	m_heap = heap;
	D3D12_DESCRIPTOR_HEAP_DESC desc = m_heap->GetDesc();
	m_heapType = desc.Type;
	m_heapStartCPU = m_heap->GetCPUDescriptorHandleForHeapStart();
	m_heapStartGPU = m_heap->GetGPUDescriptorHandleForHeapStart();
	m_heapHandleIncrement = device->GetDescriptorHandleIncrementSize(m_heapType);
	m_freeIndices.reserve((int)desc.NumDescriptors);
	for (int n = desc.NumDescriptors; n > 0; n--)
	{
		m_freeIndices.push_back(n - 1);
	}
}
void ImGUIHeapAllocator::Destroy()
{
	m_heap = nullptr;
	m_freeIndices.clear();
}
void ImGUIHeapAllocator::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
{
	IM_ASSERT(m_freeIndices.size() > 0);
	int idx = m_freeIndices.back();
	out_cpu_desc_handle->ptr = m_heapStartCPU.ptr + (idx * m_heapHandleIncrement);
	out_gpu_desc_handle->ptr = m_heapStartGPU.ptr + (idx * m_heapHandleIncrement);
}
void ImGUIHeapAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
{
	int cpu_idx = (int)((out_cpu_desc_handle.ptr - m_heapStartCPU.ptr) / m_heapHandleIncrement);
	int gpu_idx = (int)((out_gpu_desc_handle.ptr - m_heapStartGPU.ptr) / m_heapHandleIncrement);
	GUARANTEE_OR_DIE(cpu_idx == gpu_idx, " ImGui cpu_idk != gpu_idx");
	m_freeIndices.push_back(cpu_idx);
}
#endif // RENDERER_DX12
