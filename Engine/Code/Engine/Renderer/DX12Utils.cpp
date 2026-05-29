#include "Engine/Renderer/DX12Utils.hpp"
#include "Engine/Renderer/RendererDX12.hpp"

HRESULT D3DX12SerializeVersionedRootSignature(D3D12_VERSIONED_ROOT_SIGNATURE_DESC const* pRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION MaxVersion, ID3DBlob** ppBlob, ID3DBlob** ppErrorBlob)
{
	if (ppErrorBlob != NULL)
	{
		*ppErrorBlob = NULL;
	}

	switch (MaxVersion)
	{
	case D3D_ROOT_SIGNATURE_VERSION_1_0:
		switch (pRootSignatureDesc->Version)
		{
		case D3D_ROOT_SIGNATURE_VERSION_1_0:
			return D3D12SerializeRootSignature(&pRootSignatureDesc->Desc_1_0, D3D_ROOT_SIGNATURE_VERSION_1, ppBlob, ppErrorBlob);

		case D3D_ROOT_SIGNATURE_VERSION_1_1:
		{
			HRESULT hr = S_OK;
			const D3D12_ROOT_SIGNATURE_DESC1& desc_1_1 = pRootSignatureDesc->Desc_1_1;

			const SIZE_T ParametersSize = sizeof(D3D12_ROOT_PARAMETER) * desc_1_1.NumParameters;
			void* pParameters = (ParametersSize > 0) ? HeapAlloc(GetProcessHeap(), 0, ParametersSize) : NULL;
			if (ParametersSize > 0 && pParameters == NULL)
			{
				hr = E_OUTOFMEMORY;
			}
			D3D12_ROOT_PARAMETER* pParameters_1_0 = reinterpret_cast<D3D12_ROOT_PARAMETER*>(pParameters);

			if (SUCCEEDED(hr))
			{
				for (unsigned int n = 0; n < desc_1_1.NumParameters; n++)
				{
					__analysis_assume(ParametersSize == sizeof(D3D12_ROOT_PARAMETER) * desc_1_1.NumParameters);
					pParameters_1_0[n].ParameterType = desc_1_1.pParameters[n].ParameterType;
					pParameters_1_0[n].ShaderVisibility = desc_1_1.pParameters[n].ShaderVisibility;

					switch (desc_1_1.pParameters[n].ParameterType)
					{
					case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
						pParameters_1_0[n].Constants.Num32BitValues = desc_1_1.pParameters[n].Constants.Num32BitValues;
						pParameters_1_0[n].Constants.RegisterSpace = desc_1_1.pParameters[n].Constants.RegisterSpace;
						pParameters_1_0[n].Constants.ShaderRegister = desc_1_1.pParameters[n].Constants.ShaderRegister;
						break;

					case D3D12_ROOT_PARAMETER_TYPE_CBV:
					case D3D12_ROOT_PARAMETER_TYPE_SRV:
					case D3D12_ROOT_PARAMETER_TYPE_UAV:
						pParameters_1_0[n].Descriptor.RegisterSpace = desc_1_1.pParameters[n].Descriptor.RegisterSpace;
						pParameters_1_0[n].Descriptor.ShaderRegister = desc_1_1.pParameters[n].Descriptor.ShaderRegister;
						break;

					case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
						const D3D12_ROOT_DESCRIPTOR_TABLE1& table_1_1 = desc_1_1.pParameters[n].DescriptorTable;

						const SIZE_T DescriptorRangesSize = sizeof(D3D12_DESCRIPTOR_RANGE) * table_1_1.NumDescriptorRanges;
						void* pDescriptorRanges = (DescriptorRangesSize > 0 && SUCCEEDED(hr)) ? HeapAlloc(GetProcessHeap(), 0, DescriptorRangesSize) : NULL;
						if (DescriptorRangesSize > 0 && pDescriptorRanges == NULL)
						{
							hr = E_OUTOFMEMORY;
						}
						D3D12_DESCRIPTOR_RANGE* pDescriptorRanges_1_0 = reinterpret_cast<D3D12_DESCRIPTOR_RANGE*>(pDescriptorRanges);

						if (SUCCEEDED(hr))
						{
							for (unsigned int x = 0; x < table_1_1.NumDescriptorRanges; x++)
							{
								__analysis_assume(DescriptorRangesSize == sizeof(D3D12_DESCRIPTOR_RANGE) * table_1_1.NumDescriptorRanges);
								pDescriptorRanges_1_0[x].BaseShaderRegister = table_1_1.pDescriptorRanges[x].BaseShaderRegister;
								pDescriptorRanges_1_0[x].NumDescriptors = table_1_1.pDescriptorRanges[x].NumDescriptors;
								pDescriptorRanges_1_0[x].OffsetInDescriptorsFromTableStart = table_1_1.pDescriptorRanges[x].OffsetInDescriptorsFromTableStart;
								pDescriptorRanges_1_0[x].RangeType = table_1_1.pDescriptorRanges[x].RangeType;
								pDescriptorRanges_1_0[x].RegisterSpace = table_1_1.pDescriptorRanges[x].RegisterSpace;
							}
						}

						D3D12_ROOT_DESCRIPTOR_TABLE& table_1_0 = pParameters_1_0[n].DescriptorTable;
						table_1_0.NumDescriptorRanges = table_1_1.NumDescriptorRanges;
						table_1_0.pDescriptorRanges = pDescriptorRanges_1_0;
					}
				}
			}

			if (SUCCEEDED(hr))
			{
				D3D12_ROOT_SIGNATURE_DESC desc_1_0 = {};
				desc_1_0.NumParameters = desc_1_1.NumParameters;
				desc_1_0.pParameters = pParameters_1_0;
				desc_1_0.pStaticSamplers = desc_1_1.pStaticSamplers;
				desc_1_0.Flags = desc_1_1.Flags;
				hr = D3D12SerializeRootSignature(&desc_1_0, D3D_ROOT_SIGNATURE_VERSION_1, ppBlob, ppErrorBlob);
			}

			if (pParameters)
			{
				for (unsigned int n = 0; n < desc_1_1.NumParameters; n++)
				{
					if (desc_1_1.pParameters[n].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
					{
						HeapFree(GetProcessHeap(), 0, reinterpret_cast<void*>(const_cast<D3D12_DESCRIPTOR_RANGE*>(pParameters_1_0[n].DescriptorTable.pDescriptorRanges)));
					}
				}
				HeapFree(GetProcessHeap(), 0, pParameters);
			}
			return hr;
		}
		}
		break;

	case D3D_ROOT_SIGNATURE_VERSION_1_1:
		return D3D12SerializeVersionedRootSignature(pRootSignatureDesc, ppBlob, ppErrorBlob);
	}

	return E_INVALIDARG;
}

UINT64 UpdateSubresources(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDestinationResource, ID3D12Resource* pIntermediate, UINT64 intermediateOffset, unsigned int firstSubresource, unsigned int numSubresources, D3D12_SUBRESOURCE_DATA* pSrcData)
{
	UINT64 requiredSize = 0;
	UINT64 memToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * numSubresources;
	if (memToAlloc > SIZE_MAX)
	{
		return 0;
	}
	void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(memToAlloc));
	if (pMem == NULL)
	{
		return 0;
	}
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
	UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + numSubresources);
	UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + numSubresources);

	D3D12_RESOURCE_DESC desc = pDestinationResource->GetDesc();
	ID3D12Device* pDevice = nullptr;
	pDestinationResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
	pDevice->GetCopyableFootprints(&desc, firstSubresource, numSubresources, intermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &requiredSize);
	DX_SAFE_RELEASE(pDevice);

	UINT64 result = UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, firstSubresource, numSubresources, requiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData);
	HeapFree(GetProcessHeap(), 0, pMem);

	return result;
}

UINT64 UpdateSubresources(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDestinationResource, ID3D12Resource* pIntermediate, unsigned int FirstSubresource, unsigned int numSubresources, UINT64 requiredSize, D3D12_PLACED_SUBRESOURCE_FOOTPRINT const* pLayouts, unsigned int const* pNumRows, UINT64 const* pRowSizesInBytes, D3D12_SUBRESOURCE_DATA const* pSrcData)
{
	// Minor validation
	D3D12_RESOURCE_DESC IntermediateDesc = pIntermediate->GetDesc();
	D3D12_RESOURCE_DESC DestinationDesc = pDestinationResource->GetDesc();
	if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
		IntermediateDesc.Width < requiredSize + pLayouts[0].Offset ||
		requiredSize >(SIZE_T) - 1 ||
		(DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
			(FirstSubresource != 0 || numSubresources != 1)))
	{
		return 0;
	}

	BYTE* pData;
	HRESULT hr = pIntermediate->Map(0, NULL, reinterpret_cast<void**>(&pData));
	if (FAILED(hr))
	{
		return 0;
	}

	for (UINT i = 0; i < numSubresources; ++i)
	{
		if (pRowSizesInBytes[i] > (SIZE_T)-1) return 0;
		D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, pLayouts[i].Footprint.RowPitch * pNumRows[i] };
		MemcpySubresource(&DestData, &pSrcData[i], (SIZE_T)pRowSizesInBytes[i], pNumRows[i], pLayouts[i].Footprint.Depth);
	}
	pIntermediate->Unmap(0, NULL);

	if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		D3D12_BOX SrcBox = {
			UINT(pLayouts[0].Offset),
			0,
			0,
			UINT(pLayouts[0].Offset + pLayouts[0].Footprint.Width),
			1,
			1,
		};

		pCmdList->CopyBufferRegion(
			pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
	}
	else
	{
		for (UINT i = 0; i < numSubresources; ++i)
		{
			D3D12_TEXTURE_COPY_LOCATION Dst = {
				pDestinationResource,
				D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
				i + FirstSubresource
			};

			D3D12_TEXTURE_COPY_LOCATION Src = {
				pIntermediate,
				D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
				pLayouts[i]
			};
			pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
	}
	return requiredSize;
}


void MemcpySubresource(D3D12_MEMCPY_DEST const* pDest, D3D12_SUBRESOURCE_DATA const* pSrc, SIZE_T rowSizeInBytes, unsigned int numRows, unsigned int numSlices)
{
	for (UINT z = 0; z < numSlices; ++z)
	{
		unsigned char* pDestSlice = reinterpret_cast<unsigned char*>(pDest->pData) + pDest->SlicePitch * z;
		const unsigned char* pSrcSlice = reinterpret_cast<const unsigned char*>(pSrc->pData) + pSrc->SlicePitch * z;
		for (UINT y = 0; y < numRows; ++y)
		{
			memcpy(pDestSlice + pDest->RowPitch * y, pSrcSlice + pSrc->RowPitch * y, rowSizeInBytes);
		}
	}
}

void SetResourceName(ID3D12Resource* out_resource, std::string const& name)
{
	if(!out_resource)
		return;

	out_resource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name.c_str()) + 1, name.c_str());
}

void SetCommandQueueName(ID3D12CommandQueue* out_commandQueue, std::string const& name)
{
	if(!out_commandQueue)
		return;

	out_commandQueue->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name.c_str()) + 1, name.c_str());
}

void SetFenceName(ID3D12Fence* out_fence, std::string const& name)
{
	if(!out_fence)
		return;

	out_fence->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name.c_str()) + 1, name.c_str());
}

void SetCommandListName(ID3D12GraphicsCommandList* out_commandList, std::string const& name)
{
	if (!out_commandList)
		return;

	out_commandList->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name.c_str()) + 1, name.c_str());
}

void SetCommandAllocatorName(ID3D12CommandAllocator* out_commandAllocator, std::string const& name)
{
	if (!out_commandAllocator)
		return;

	out_commandAllocator->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name.c_str()) + 1, name.c_str());
}

void SetSwapChainName(IDXGISwapChain4* out_swapChain, std::string const& name)
{
	if (!out_swapChain)
		return;

	out_swapChain->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name.c_str()) + 1, name.c_str());
}

void SetDescriptorHeapName(ID3D12DescriptorHeap* out_descriptorHeap, std::string const& name)
{
	if (!out_descriptorHeap)
		return;

	out_descriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name.c_str()) + 1, name.c_str());
}

void SetPipelineStateName(ID3D12PipelineState* out_pipelineState, std::string const& name)
{
	if (!out_pipelineState)
		return;

	out_pipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name.c_str()) + 1, name.c_str());
}

void SetRootSignatureName(ID3D12RootSignature* out_rootSignature, std::string const& name)
{
	if (!out_rootSignature)
		return;

	out_rootSignature->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name.c_str()) + 1, name.c_str());
}

size_t AlignUp(size_t value, size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

int CalculateFullMipCount2D(int width, int height)
{
	int largestDimension = (width > height) ? width : height;
	int mipCount = 1;
	while (largestDimension > 1)
	{
		largestDimension = (largestDimension >> 1);
		mipCount = (mipCount + 1);
	}

	return mipCount;
}
