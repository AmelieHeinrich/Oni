/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:55:42
 */

#pragma once

#include "device.hpp"
#include "descriptor_heap.hpp"
#include "allocator.hpp"

enum class TextureFormat
{
    RGBA8 = DXGI_FORMAT_R8G8B8A8_UNORM,
    RGBA32Float = DXGI_FORMAT_R32G32B32A32_FLOAT,
    RGBA16Float = DXGI_FORMAT_R16G16B16A16_FLOAT,
    R32Depth = DXGI_FORMAT_D32_FLOAT
};

enum class TextureLayout
{
    Common = D3D12_RESOURCE_STATE_COMMON,
    ShaderResource = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
    Storage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    Depth = D3D12_RESOURCE_STATE_DEPTH_WRITE,
    RenderTarget = D3D12_RESOURCE_STATE_RENDER_TARGET,
    CopySource = D3D12_RESOURCE_STATE_COPY_SOURCE,
    CopyDest = D3D12_RESOURCE_STATE_COPY_DEST,
    Present = D3D12_RESOURCE_STATE_PRESENT,
    DataRead = D3D12_RESOURCE_STATE_GENERIC_READ,
    DataWrite = D3D12_RESOURCE_STATE_COMMON
};

enum class TextureUsage
{
    Copy = D3D12_RESOURCE_FLAG_NONE,
    RenderTarget = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
    DepthTarget = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
    Storage = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
    ShaderResource = 8
};

class Texture
{
public:
    using Ptr = std::shared_ptr<Texture>;

    Texture(Device::Ptr devicePtr, Allocator::Ptr allocator);
    Texture(Device::Ptr devicePtr, Allocator::Ptr allocator, uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage);
    ~Texture();

    void BuildRenderTarget(DescriptorHeap::Ptr heap);
    void BuildDepthTarget(DescriptorHeap::Ptr heap);
    void BuildShaderResource(DescriptorHeap::Ptr heap);
    void BuildStorage(DescriptorHeap::Ptr heap);

    void SetState(D3D12_RESOURCE_STATES state) { _state = state; }

private:
    Device::Ptr _devicePtr;
    DescriptorHeap::Ptr _rtvHeap;
    DescriptorHeap::Ptr _dsvHeap;
    DescriptorHeap::Ptr _shaderHeap;

    bool _release = true;

    GPUResource _resource;

    D3D12_RESOURCE_STATES _state;
    
    DescriptorHeap::Descriptor _rtv;
    DescriptorHeap::Descriptor _dsv;
    DescriptorHeap::Descriptor _srvUav;

    TextureFormat _format;
};
