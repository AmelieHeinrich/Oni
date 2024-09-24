/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 23:32:49
 */

#pragma once

#include "allocator.hpp"
#include "texture.hpp"
#include "descriptor_heap.hpp"

typedef TextureLayout BufferLayout;

enum class BufferType
{
    Vertex,
    Index,
    Constant,
    Storage,
    Copy
};

class Buffer
{
public:
    using Ptr = std::shared_ptr<Buffer>;

    Buffer(Device::Ptr device, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, uint64_t size, uint64_t stride, BufferType type, bool readback, const std::string& name = "Buffer");
    ~Buffer();

    void BuildConstantBuffer();
    void BuildStorage();

    void Map(int start, int end, void **data);
    void Unmap(int start, int end);

    void SetState(D3D12_RESOURCE_STATES eState) { _state = eState; }

    uint32_t CBV() { return _cbv.HeapIndex; }
    uint32_t UAV() { return _uav.HeapIndex; }

    // TODO: SRVs for StructuredBuffer (used in raytracing and a bunch of other stuff iirc)
    // uint32_t SRV() { return _cbv.HeapIndex; }
private:
    friend class CommandBuffer;

    Device::Ptr _devicePtr;
    DescriptorHeap::Heaps _heaps;

    BufferType _type;
    uint64_t _size;

    DescriptorHeap::Descriptor _cbv;
    DescriptorHeap::Descriptor _uav;

    GPUResource *_resource = nullptr;
    D3D12_RESOURCE_STATES _state;

    D3D12_VERTEX_BUFFER_VIEW _VBV;
    D3D12_INDEX_BUFFER_VIEW _IBV;
    D3D12_CONSTANT_BUFFER_VIEW_DESC _CBVD;
    D3D12_UNORDERED_ACCESS_VIEW_DESC _UAVD;
};
