#include "Engine/Renderer/BufferDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"


BufferDX12::BufferDX12(RendererDX12 const* renderer, std::string const& name)
	:ResourceDX12(renderer, name)
{
}


BufferDX12::~BufferDX12()
{
}

BufferDX12::BufferDX12(RendererDX12 const* renderer, D3D12_RESOURCE_DESC const& resourceDesc, std::string const& name)
	:ResourceDX12(renderer, resourceDesc, nullptr, name)
{
}

void BufferDX12::CreateViews(size_t numElements, size_t elementSize)
{
	UNUSED(numElements);
	UNUSED(elementSize);
	ERROR_AND_DIE("BufferDX12::CreateViews() should not be called");
}


