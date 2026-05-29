#include "Engine/Renderer/RootSignatureDX12.hpp"
#include "Engine/Renderer/RendererDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <cassert>

RootSignatureDX12::RootSignatureDX12(RendererDX12 const* renderer,const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion, std::string const& name)
	: m_renderer(renderer)
	, m_rootSignatureDesc{}
	, m_numDescriptorsPerTable{ 0 }
	, m_samplerTableBitMask(0)
	, m_descriptorTableBitMask(0)
	, m_name(name)
{
	SetRootSignatureDesc(rootSignatureDesc, rootSignatureVersion);
	SetRootSignatureName(m_rootSignature, name);
}

RootSignatureDX12::~RootSignatureDX12()
{
	Destroy();
}

void RootSignatureDX12::Destroy()
{
	for (unsigned int i = 0; i < m_rootSignatureDesc.NumParameters; ++i)
	{
		const D3D12_ROOT_PARAMETER1& rootParameter = m_rootSignatureDesc.pParameters[i];
		if (rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			delete[] rootParameter.DescriptorTable.pDescriptorRanges;
		}
	}

	delete[] m_rootSignatureDesc.pParameters;
	m_rootSignatureDesc.pParameters = nullptr;
	m_rootSignatureDesc.NumParameters = 0;

	delete[] m_rootSignatureDesc.pStaticSamplers;
	m_rootSignatureDesc.pStaticSamplers = nullptr;
	m_rootSignatureDesc.NumStaticSamplers = 0;

	m_descriptorTableBitMask = 0;
	m_samplerTableBitMask = 0;

	memset(m_numDescriptorsPerTable, 0, sizeof(m_numDescriptorsPerTable));

	DX_SAFE_RELEASE(m_rootSignature);
}

void RootSignatureDX12::SetRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion)
{
	// Make sure any previously allocated root signature description is cleaned up first.
	Destroy();

	ID3D12Device* device = m_renderer->GetDevice();

	unsigned int numParameters = rootSignatureDesc.NumParameters;
	D3D12_ROOT_PARAMETER1* pParameters = numParameters > 0 ? new D3D12_ROOT_PARAMETER1[numParameters] : nullptr;

	for (unsigned int i = 0; i < numParameters; ++i)
	{
		const D3D12_ROOT_PARAMETER1& rootParameter = rootSignatureDesc.pParameters[i];
		pParameters[i] = rootParameter;

		if (rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			unsigned int numDescriptorRanges = rootParameter.DescriptorTable.NumDescriptorRanges;
			D3D12_DESCRIPTOR_RANGE1* pDescriptorRanges = numDescriptorRanges > 0 ? new D3D12_DESCRIPTOR_RANGE1[numDescriptorRanges] : nullptr;

			memcpy(pDescriptorRanges, rootParameter.DescriptorTable.pDescriptorRanges,
				sizeof(D3D12_DESCRIPTOR_RANGE1) * numDescriptorRanges);

			pParameters[i].DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
			pParameters[i].DescriptorTable.pDescriptorRanges = pDescriptorRanges;

			// Set the bit mask depending on the type of descriptor table.
			if (numDescriptorRanges > 0)
			{
				switch (pDescriptorRanges[0].RangeType)
				{
				case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
				case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
				case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
					m_descriptorTableBitMask |= (1 << i);
					break;
				case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
					m_samplerTableBitMask |= (1 << i);
					break;
				}
			}

			// Count the number of descriptors in the descriptor table.
			for (unsigned int j = 0; j < numDescriptorRanges; ++j)
			{
				if (pDescriptorRanges[j].NumDescriptors >= MAX_NUM_BINDLESS_TEXTURES)
				{
					m_numDescriptorsPerTable[i] = 0;//MAX_NUM_BINDLESS_TEXTURES;
				}

				else
				{
					m_numDescriptorsPerTable[i] += pDescriptorRanges[j].NumDescriptors;
				}
			}
		}
	}

	m_rootSignatureDesc.NumParameters = numParameters;
	m_rootSignatureDesc.pParameters = pParameters;

	unsigned int numStaticSamplers = rootSignatureDesc.NumStaticSamplers;
	D3D12_STATIC_SAMPLER_DESC* pStaticSamplers = numStaticSamplers > 0 ? new D3D12_STATIC_SAMPLER_DESC[numStaticSamplers] : nullptr;

	if (pStaticSamplers)
	{
		memcpy(pStaticSamplers, rootSignatureDesc.pStaticSamplers,
			sizeof(D3D12_STATIC_SAMPLER_DESC) * numStaticSamplers);
	}

	m_rootSignatureDesc.NumStaticSamplers = numStaticSamplers;
	m_rootSignatureDesc.pStaticSamplers = pStaticSamplers;

	D3D12_ROOT_SIGNATURE_FLAGS flags = rootSignatureDesc.Flags;
	m_rootSignatureDesc.Flags = flags;

	// --- Final root signature description ---
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionRootSignatureDesc = {};
	versionRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	versionRootSignatureDesc.Desc_1_1.NumParameters = numParameters;
	versionRootSignatureDesc.Desc_1_1.pParameters = pParameters;
	versionRootSignatureDesc.Desc_1_1.NumStaticSamplers = numStaticSamplers;
	versionRootSignatureDesc.Desc_1_1.pStaticSamplers = pStaticSamplers;
	versionRootSignatureDesc.Desc_1_1.Flags = flags;

	// Serialize the root signature.
	ID3DBlob* rootSignatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3DX12SerializeVersionedRootSignature(&versionRootSignatureDesc, rootSignatureVersion, &rootSignatureBlob, &errorBlob);
	if (!SUCCEEDED(hr))
	{
		if (errorBlob != NULL)
		{
			DebuggerPrintf((char*)(errorBlob->GetBufferPointer()));
		}
		ERROR_AND_DIE("Could not serialize versioned root signature");
	}

	if (errorBlob != NULL)
	{
		DX_SAFE_RELEASE(errorBlob);
	}

	// Create the root signature.
	hr = device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	if (!SUCCEEDED(hr))
	{
		if (errorBlob != NULL)
		{
			DebuggerPrintf((char*)(errorBlob->GetBufferPointer()));
		}
		ERROR_AND_DIE("Could not create root signature");
	}

	if (errorBlob != NULL)
	{
		DX_SAFE_RELEASE(errorBlob);
	}

	DX_SAFE_RELEASE(rootSignatureBlob);
}

uint32_t RootSignatureDX12::GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const
{
	uint32_t descriptorTableBitMask = 0;
	switch (descriptorHeapType)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		descriptorTableBitMask = m_descriptorTableBitMask;
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
		descriptorTableBitMask = m_samplerTableBitMask;
		break;
	}

	return descriptorTableBitMask;
}

uint32_t RootSignatureDX12::GetNumDescriptors(uint32_t rootIndex) const
{
	assert(rootIndex < 32);
	return m_numDescriptorsPerTable[rootIndex];
}
