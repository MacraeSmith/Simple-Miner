#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dCompiler.lib")

IndexBuffer::IndexBuffer(ID3D11Device* device, unsigned int size)
	:m_device(device)
	,m_size(size)
	,m_stride(sizeof(unsigned int))
{
	Create();
}

IndexBuffer::~IndexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
	m_device = nullptr;
}

void IndexBuffer::Create()
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = m_size;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr;
	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create new index buffer");
	}
}

void IndexBuffer::Resize(unsigned int size)
{
	DX_SAFE_RELEASE(m_buffer);

	m_size = size;
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = m_size;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr;
	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create new buffer while resizing");
	}
}

unsigned int IndexBuffer::GetStride() const
{
	return m_stride;
}

unsigned int IndexBuffer::GetSize() const
{
	return m_size;
}

unsigned int IndexBuffer::GetNumIndexes() const
{
	return m_size / m_stride;
}
