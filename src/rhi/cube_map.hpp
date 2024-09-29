/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 00:45:09
 */

#pragma once

#include "texture.hpp"

#include <vector>

class CubeMap
{
public:
    using Ptr = std::shared_ptr<CubeMap>;

    CubeMap(Device::Ptr devicePtr, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, uint32_t width, uint32_t height, TextureFormat format, int mips = 1, const std::string& name = "Cube Map");
    ~CubeMap();

    void SetState(D3D12_RESOURCE_STATES state) { _state = state; }
    D3D12_RESOURCE_STATES GetState() { return _state; }

    GPUResource& GetResource() { return *_resource; }
    int GetMips() { return _mips; }

    uint32_t SRV() { return _srv.HeapIndex; }
    uint32_t UAV(uint32_t mip = 0) {return _uavs[mip].HeapIndex; }
private:
    friend class CommandBuffer;

    Device::Ptr _devicePtr;
    DescriptorHeap::Heaps _heaps;

    GPUResource *_resource;

    D3D12_RESOURCE_STATES _state;

    DescriptorHeap::Descriptor _srv;
    std::vector<DescriptorHeap::Descriptor> _uavs;

    TextureFormat _format;
    int _width;
    int _height;
    int _mips;
};
