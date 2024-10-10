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
    Copy,
    AccelerationStructure
};

class Buffer
{
public:
    using Ptr = std::shared_ptr<Buffer>;

    Buffer(Device::Ptr device, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, uint64_t size, uint64_t stride, BufferType type, bool readback, const std::string& name = "Buffer");
    ~Buffer();

    void BuildConstantBuffer();
    void BuildStorage();
    void BuildShaderResource();

    void Map(int start, int end, void **data);
    void Unmap(int start, int end);

    void SetState(D3D12_RESOURCE_STATES eState) { _state = eState; }

    uint32_t CBV() { return _cbv.HeapIndex; }
    uint32_t UAV() { return _uav.HeapIndex; }
    uint32_t SRV() { return _srv.HeapIndex; }
private:
    friend class BLAS;
    friend class TLAS;
    friend class CommandBuffer;

    Device::Ptr _devicePtr;
    DescriptorHeap::Heaps _heaps;

    BufferType _type;
    uint64_t _size;
    uint64_t _stride;

    DescriptorHeap::Descriptor _cbv;
    DescriptorHeap::Descriptor _uav;
    DescriptorHeap::Descriptor _srv;

    GPUResource *_resource = nullptr;
    D3D12_RESOURCE_STATES _state;

    D3D12_VERTEX_BUFFER_VIEW _VBV;
    D3D12_INDEX_BUFFER_VIEW _IBV;
    D3D12_CONSTANT_BUFFER_VIEW_DESC _CBVD;
    D3D12_UNORDERED_ACCESS_VIEW_DESC _UAVD;
};
