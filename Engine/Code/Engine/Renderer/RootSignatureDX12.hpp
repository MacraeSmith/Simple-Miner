#pragma once
#include <d3d12.h>
#include <cstdint>
#include <string>
struct ID3D12RootSignature;
class RendererDX12;
class RootSignatureDX12
{
public:
	RootSignatureDX12(RendererDX12 const* renderer, D3D12_ROOT_SIGNATURE_DESC1 const& rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion, std::string const& name = "RootSignatureDX12");
	~RootSignatureDX12();

	void				Destroy();
	void				SetRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc,D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion);

	ID3D12RootSignature* GetRootSignature() const{return m_rootSignature;}
	const D3D12_ROOT_SIGNATURE_DESC1& GetRootSignatureDesc() const{return m_rootSignatureDesc;}
	uint32_t		GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;
	uint32_t		GetNumDescriptors(uint32_t rootIndex) const;
	std::string		GetName() const {return m_name;}

protected:

private:
	RendererDX12 const* m_renderer = nullptr;
	std::string m_name;
	D3D12_ROOT_SIGNATURE_DESC1 m_rootSignatureDesc;
	ID3D12RootSignature* m_rootSignature = nullptr;

	// Need to know the number of descriptors per descriptor table.
	// A maximum of 32 descriptor tables are supported (since a 32-bit
	// mask is used to represent the descriptor tables in the root signature.
	uint32_t m_numDescriptorsPerTable[32];

	// A bit mask that represents the root parameter indices that are 
	// descriptor tables for Samplers.
	uint32_t m_samplerTableBitMask;
	// A bit mask that represents the root parameter indices that are 
	// CBV, UAV, and SRV descriptor tables.
	uint32_t m_descriptorTableBitMask;

};

