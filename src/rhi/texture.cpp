/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 23:04:25
 */

#include "texture.hpp"

D3D12_RESOURCE_FLAGS GetResourceFlag(TextureUsage usage)
{
    switch (usage)
    {
        case TextureUsage::RenderTarget:
            return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        case TextureUsage::DepthTarget:
            return D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        case TextureUsage::Copy:
            return D3D12_RESOURCE_FLAG_NONE;
        case TextureUsage::ShaderResource:
        case TextureUsage::Storage:
            return D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    return D3D12_RESOURCE_FLAG_NONE;
}

Texture::Texture(Device::Ptr devicePtr)
    : _release(false), _devicePtr(devicePtr)
{
}

Texture::Texture(Device::Ptr devicePtr, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage)
    : _devicePtr(devicePtr), _heaps(heaps), _format(format), _width(width), _height(height)
{
    switch (usage)
    {
        case TextureUsage::RenderTarget:
            _state = D3D12_RESOURCE_STATE_RENDER_TARGET;
            break;
        case TextureUsage::DepthTarget:
            _state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            break;
        case TextureUsage::Storage:
            _state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            break;
        case TextureUsage::ShaderResource:
            _state = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
            break;
        case TextureUsage::Copy:
            _state = D3D12_RESOURCE_STATE_COPY_DEST;
            break;
    }

    D3D12MA::ALLOCATION_DESC AllocationDesc = {};
    AllocationDesc.HeapType = usage == TextureUsage::Copy ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    ResourceDesc.Width = width;
    ResourceDesc.Height = height;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT(format);
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ResourceDesc.Flags = GetResourceFlag(usage);

    _resource = allocator->Allocate(&AllocationDesc, &ResourceDesc, _state);
}

Texture::~Texture()
{
    if (_release) {
        if (_uav.Valid) {
            _heaps.ShaderHeap->Free(_uav);
        }
        if (_srv.Valid) {
            _heaps.ShaderHeap->Free(_srv);
        }
        if (_dsv.Valid) {
            _heaps.DSVHeap->Free(_dsv);
        }
        if (_rtv.Valid) {
            _heaps.RTVHeap->Free(_rtv);
        }
        _resource.Allocation->Release();
    }
}

void Texture::BuildRenderTarget()
{
    _rtv = _heaps.RTVHeap->Allocate();

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT(_format);
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    _devicePtr->GetDevice()->CreateRenderTargetView(_resource.Resource, &rtvDesc, _rtv.CPU);
}

void Texture::BuildDepthTarget()
{
    _dsv = _heaps.DSVHeap->Allocate();

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT(_format);
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    _devicePtr->GetDevice()->CreateDepthStencilView(_resource.Resource, &dsvDesc, _dsv.CPU);
}

void Texture::BuildShaderResource()
{
    _srv = _heaps.ShaderHeap->Allocate();
    
    D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceView = {};
    ShaderResourceView.Format = DXGI_FORMAT(_format);
    ShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    ShaderResourceView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    ShaderResourceView.Texture2D.MipLevels = 1;

    _devicePtr->GetDevice()->CreateShaderResourceView(_resource.Resource, &ShaderResourceView, _srv.CPU);
}

void Texture::BuildStorage()
{
    _uav = _heaps.ShaderHeap->Allocate();
    
    D3D12_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessView = {};
    UnorderedAccessView.Format = DXGI_FORMAT(_format);
    UnorderedAccessView.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    _devicePtr->GetDevice()->CreateUnorderedAccessView(_resource.Resource, nullptr, &UnorderedAccessView, _uav.CPU);
}
