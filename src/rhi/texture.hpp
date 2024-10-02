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
    None = DXGI_FORMAT_UNKNOWN,
    RGBA8 = DXGI_FORMAT_R8G8B8A8_UNORM,
    RGBA32Float = DXGI_FORMAT_R32G32B32A32_FLOAT,
    RGBA16Float = DXGI_FORMAT_R16G16B16A16_FLOAT,
    RGBA16Unorm = DXGI_FORMAT_R16G16B16A16_UNORM,
    RGB11Float = DXGI_FORMAT_R11G11B10_FLOAT,
    RG16Float = DXGI_FORMAT_R16G16_FLOAT,
    R32Float = DXGI_FORMAT_R32_FLOAT,
    R32Depth = DXGI_FORMAT_D32_FLOAT,
    BC1 = DXGI_FORMAT_BC1_UNORM,
    BC7 = DXGI_FORMAT_BC7_UNORM,
    R32Typeless = DXGI_FORMAT_R32_TYPELESS // For shadows!
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
    Copy,
    RenderTarget,
    DepthTarget,
    Storage,
    ShaderResource,
    RenderStorage
};

class Texture
{
public:
    using Ptr = std::shared_ptr<Texture>;

    Texture(Device::Ptr devicePtr, const std::string& name = "Texture");
    Texture(Device::Ptr devicePtr, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, bool mips, const std::string& name = "Texture");
    ~Texture();

    void BuildRenderTarget(TextureFormat specificFormat = TextureFormat::None);
    void BuildDepthTarget(TextureFormat specificFormat = TextureFormat::None);
    void BuildShaderResource(TextureFormat specificFormat = TextureFormat::None);
    void BuildStorage(TextureFormat specificFormat = TextureFormat::None);

    void SetState(D3D12_RESOURCE_STATES state, int mip);
    D3D12_RESOURCE_STATES GetState(int mip);

    GPUResource& GetResource() { return *_resource; }

    static float GetComponentSize(TextureFormat format);
    TextureFormat GetFormat() { return _format; }

    D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiImage() { return _srvs.front().GPU; }

    int GetWidth() { return _width; }
    int GetHeight() { return _height; }
    int GetMips() { return _mipLevels; }

    uint32_t SRV(uint32_t mip = 0) { return _srvs[mip].HeapIndex; }
    uint32_t UAV(uint32_t mip = 0) { return _uavs[mip].HeapIndex; }
    
    int GetSizeOfMip(uint32_t mip) { return _mipSizes[mip]; }
private:
    friend class SwapChain;
    friend class CommandBuffer;
    friend class Allocator;

    Device::Ptr _devicePtr;
    DescriptorHeap::Heaps _heaps;

    bool _release = true;

    GPUResource *_resource;
    
    DescriptorHeap::Descriptor _rtv;
    DescriptorHeap::Descriptor _dsv;

    std::vector<int> _mipSizes; // Used only for uploader kekw
    std::vector<DescriptorHeap::Descriptor> _srvs;
    std::vector<DescriptorHeap::Descriptor> _uavs;
    std::vector<D3D12_RESOURCE_STATES> _states;

    TextureFormat _format;
    int _width;
    int _height;
    int _mipLevels;
};
