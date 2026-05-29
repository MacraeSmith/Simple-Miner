#pragma once
#include <string>
#include <d3d12.h>
#include <memory>

class RendererDX12;
class CommandQueue;
class ResourceDX12
{
public:
    ResourceDX12(RendererDX12 const* renderer, std::string const& name = "AddedResource");
    ResourceDX12(RendererDX12 const* renderer, D3D12_RESOURCE_DESC const& resourceDesc,const D3D12_CLEAR_VALUE* clearValue = nullptr, std::string const& name = "AddedResource");
    ResourceDX12(RendererDX12 const* renderer, ID3D12Resource* resource, std::string const& name = "AddedResource");
    
	ResourceDX12(const ResourceDX12&) = delete;
	ResourceDX12& operator=(const ResourceDX12&) = delete;

	ResourceDX12(ResourceDX12&&) = delete;
	ResourceDX12& operator=(ResourceDX12&&) = delete;
    

    virtual ~ResourceDX12();

    
    //Check to see if the underlying resource is valid.
    bool IsValid() const
    {
        return (m_d3d12Resource != nullptr);
    }

    // Get access to the underlying D3D12 resource
    ID3D12Resource* GetD3D12Resource() const { return m_d3d12Resource;}

    D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const
    {
        D3D12_RESOURCE_DESC resDesc = {};
        if (m_d3d12Resource)
        {
            resDesc = m_d3d12Resource->GetDesc();
        }

        return resDesc;
    }

    // Replace the D3D12 resource. Should only be called by the CommandList.
    virtual void SetD3D12Resource(ID3D12Resource* d3d12Resource, const D3D12_CLEAR_VALUE* clearValue = nullptr);

    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const;


    //Get the UAV for a (sub)resource. @param uavDesc The description of the UAV to return.
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const;

    void        SetName(std::string const& name);
    std::string GetName() const {return m_resourceName;}

    //Release the underlying resource. This is useful for swap chain resizing.
    virtual void Reset();

	bool CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const;
	bool CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const;
    std::size_t HashSRVDesc(D3D12_SHADER_RESOURCE_VIEW_DESC const& srvDesc) const;
    std::size_t HashUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC const& uavDesc) const;

    void MarkUsed(CommandQueue* commandQueueThatUsed, uint64_t const& fenceValue);

private:
    void CheckFeatureSupport();

protected:
    // The underlying D3D12 resource.
    RendererDX12 const* m_renderer = nullptr;
    ID3D12Resource* m_d3d12Resource = nullptr;
    std::unique_ptr<D3D12_CLEAR_VALUE> m_d3d12ClearValue;
    std::string m_resourceName;
    D3D12_FEATURE_DATA_FORMAT_SUPPORT m_FormatSupport;

    uint64_t m_lastUsedFenceValue = 0;
    CommandQueue* m_lastUsedCommandQueue = nullptr;
};
