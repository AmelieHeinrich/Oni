/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 23:32:49
 */

#pragma once

#include "allocator.hpp"
#include "texture.hpp"

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
    Buffer(Allocator::Ptr allocator, uint64_t size, uint64_t stride, BufferType type, bool readback);
    ~Buffer();

    void BuildConstantBuffer(Device::Ptr device, DescriptorHeap::Ptr heap);
    void BuildStorage(Device::Ptr device, DescriptorHeap::Ptr heap);

    void Map(int start, int end, void **data);
    void Unmap(int start, int end);

    void SetState(D3D12_RESOURCE_STATES eState) { _state = eState; }
private:
    DescriptorHeap::Ptr _heap;

    BufferType _type;
    uint64_t _size;

    DescriptorHeap::Descriptor _descriptor;
    GPUResource _resource;
    D3D12_RESOURCE_STATES _state;

    D3D12_VERTEX_BUFFER_VIEW _VBV;
    D3D12_INDEX_BUFFER_VIEW _IBV;
    D3D12_CONSTANT_BUFFER_VIEW_DESC _CBVD;
    D3D12_UNORDERED_ACCESS_VIEW_DESC _UAVD;
};
