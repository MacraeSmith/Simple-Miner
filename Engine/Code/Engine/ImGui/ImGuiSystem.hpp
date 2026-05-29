#pragma once
#include "Game/EngineBuildPreferences.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "ThirdParty/imgui/imgui.h"
#ifdef RENDERER_DX12

#include <d3d12.h>
class RendererDX12;
struct ImGUIHeapAllocator
{
	ID3D12DescriptorHeap* m_heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE  m_heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_heapStartCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE m_heapStartGPU;
	unsigned int m_heapHandleIncrement = 0;
	ImVector<int>	m_freeIndices;

	void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap);
	void Destroy();
	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle);
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle);
};

#else
class RendererDX11;
#endif // RENDERER_DX12


class Window;
struct ImGuiConfig
{
	#ifdef RENDERER_DX12
	RendererDX12 const* m_renderer = nullptr;
	#else
	RendererDX11 const* m_renderer = nullptr;
	#endif // RENDERER_DX12

	Window* m_window = nullptr;
	bool m_darkMode = true;
};

class ImGuiSystem
{
public:
	ImGuiSystem(ImGuiConfig config);

	~ImGuiSystem();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();
	static bool Event_ShowImGui(EventArgs& args);

private:

private:

public:
	ImGuiConfig m_config;
	bool m_isVisible = true;
	bool m_showDemoWindow = false;
#ifdef RENDERER_DX12
	ImGUIHeapAllocator m_heapAllocator;
	ID3D12DescriptorHeap* m_imguiSRVHeap = nullptr;
#endif // RENDERER_DX12


};

