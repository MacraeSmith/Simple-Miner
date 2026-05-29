#pragma once
#include "Engine/Renderer/ResourceDX12.hpp"
class RendererDX12;
class BufferDX12 : public ResourceDX12
{
public:
	BufferDX12(RendererDX12 const* renderer, std::string const& name = "AddedBuffer");
	BufferDX12(RendererDX12 const* renderer, D3D12_RESOURCE_DESC const& resourceDesc, std::string const& name = "AddedBuffer");
	virtual ~BufferDX12();

	virtual void CreateViews(size_t numElements, size_t elementSize);
};

