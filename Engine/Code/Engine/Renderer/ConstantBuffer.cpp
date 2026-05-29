#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dCompiler.lib")

ConstantBuffer::ConstantBuffer(ID3D11Device* device, size_t size)
	:m_device(device)
	,m_size(size)
{
	Create();
}

ConstantBuffer::~ConstantBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
	m_device = nullptr;
}

void ConstantBuffer::Create()
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = (unsigned int)m_size;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr;
	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create new Constant buffer");
	}

}

