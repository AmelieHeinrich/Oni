/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 23:04:25
 */

#include "texture.hpp"

#include <algorithm>

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

float Texture::GetComponentSize(TextureFormat format)
{
    switch (format)
    {
        case TextureFormat::RGB11Float: {
            return -1.0f;
        }
        case TextureFormat::RGBA16Unorm:
        case TextureFormat::RGBA16Float: {
            return 4.0f * sizeof(short);
        }
        case TextureFormat::RGBA8: {
            return 4.0f * sizeof(char);
        }
        case TextureFormat::R32Depth: {
            return 0.0f;
        }
        case TextureFormat::RGBA32Float: {
            return 4.0f * sizeof(float);
        }
        case TextureFormat::RG16Float: {
            return 2.0f * sizeof(short);
        }
        case TextureFormat::R32Float: {
            return sizeof(float);
        }
        case TextureFormat::BC1: {
            return 0.5f;
        }
        case TextureFormat::BC7: {
            return 1.0f;
        }
    }
    return 0;
}

Texture::Texture(Device::Ptr devicePtr, const std::string& name)
    : _release(false), _devicePtr(devicePtr)
{
}

Texture::Texture(Device::Ptr devicePtr, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, bool mips, const std::string& name)
    : _devicePtr(devicePtr), _heaps(heaps), _format(format), _width(width), _height(height)
{
    if (mips) {
        _mipLevels = floor(log2(max(width, height))) + 1;
    } else {
        _mipLevels = 1;
    }

    _states.resize(_mipLevels);

    switch (usage)
    {
        case TextureUsage::RenderTarget:
            std::fill(_states.begin(), _states.end(), D3D12_RESOURCE_STATE_RENDER_TARGET);
            break;
        case TextureUsage::DepthTarget:
            std::fill(_states.begin(), _states.end(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
            break;
        case TextureUsage::Storage:
            std::fill(_states.begin(), _states.end(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            break;
        case TextureUsage::ShaderResource:
            std::fill(_states.begin(), _states.end(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            break;
        case TextureUsage::Copy:
            std::fill(_states.begin(), _states.end(), D3D12_RESOURCE_STATE_COPY_DEST);
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
    ResourceDesc.Format = DXGI_FORMAT(format);
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ResourceDesc.Flags = GetResourceFlag(usage);
    ResourceDesc.MipLevels = _mipLevels;
    if (format == TextureFormat::BC1 || format == TextureFormat::BC7) {
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    }

    _resource = allocator->Allocate(&AllocationDesc, &ResourceDesc, _states[0], name);
    _resource->AttachTexture(this);
}

Texture::~Texture()
{
    if (_release) {
        for (auto& uav : _uavs) {
            if (uav.Valid) {
                _heaps.ShaderHeap->Free(uav);
            }
        }
        for (auto& srv : _srvs) {
            if (srv.Valid) {
                _heaps.ShaderHeap->Free(srv);
            }
        }
        if (_dsv.Valid) {
            _heaps.DSVHeap->Free(_dsv);
        }
        if (_rtv.Valid) {
            _heaps.RTVHeap->Free(_rtv);
        }
        _resource->Allocation->Release();
        _resource->ClearFromAllocationList();
        delete _resource;
    }
}

void Texture::BuildRenderTarget(TextureFormat specificFormat)
{
    _rtv = _heaps.RTVHeap->Allocate();

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT(_format);
    if (specificFormat != TextureFormat::None)
        rtvDesc.Format = DXGI_FORMAT(specificFormat);
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    _devicePtr->GetDevice()->CreateRenderTargetView(_resource->Resource, &rtvDesc, _rtv.CPU);
}

void Texture::BuildDepthTarget(TextureFormat specificFormat)
{
    _dsv = _heaps.DSVHeap->Allocate();

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT(_format);
    if (specificFormat != TextureFormat::None)
        dsvDesc.Format = DXGI_FORMAT(specificFormat);
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    _devicePtr->GetDevice()->CreateDepthStencilView(_resource->Resource, &dsvDesc, _dsv.CPU);
}

void Texture::BuildShaderResource(TextureFormat specificFormat)
{
    {
        auto firstMip = _heaps.ShaderHeap->Allocate();
    
        D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceView = {};
        ShaderResourceView.Format = DXGI_FORMAT(_format);
        if (specificFormat != TextureFormat::None)
            ShaderResourceView.Format = DXGI_FORMAT(specificFormat);
        ShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        ShaderResourceView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        ShaderResourceView.Texture2D.MipLevels = _mipLevels;
        ShaderResourceView.Texture2D.MostDetailedMip = 0;

        _devicePtr->GetDevice()->CreateShaderResourceView(_resource->Resource, &ShaderResourceView, firstMip.CPU);

        _srvs.push_back(firstMip);
    }

    for (int i = 1; i < _mipLevels; i++) {
        auto _srv = _heaps.ShaderHeap->Allocate();
    
        D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceView = {};
        ShaderResourceView.Format = DXGI_FORMAT(_format);
        if (specificFormat != TextureFormat::None)
            ShaderResourceView.Format = DXGI_FORMAT(specificFormat);
        ShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        ShaderResourceView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        ShaderResourceView.Texture2D.MipLevels = 1;
        ShaderResourceView.Texture2D.MostDetailedMip = i;

        _devicePtr->GetDevice()->CreateShaderResourceView(_resource->Resource, &ShaderResourceView, _srv.CPU);

        _srvs.push_back(_srv);
    }
}

void Texture::BuildStorage(TextureFormat specificFormat)
{
    for (int i = 0; i < _mipLevels; i++) {
        auto uav = _heaps.ShaderHeap->Allocate();

        D3D12_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessView = {};
        UnorderedAccessView.Format = DXGI_FORMAT(_format);
        if (specificFormat != TextureFormat::None)
            UnorderedAccessView.Format = DXGI_FORMAT(specificFormat);
        UnorderedAccessView.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        UnorderedAccessView.Texture2D.MipSlice = i;
        _devicePtr->GetDevice()->CreateUnorderedAccessView(_resource->Resource, nullptr, &UnorderedAccessView, uav.CPU);

        _uavs.push_back(uav);
    }
}
