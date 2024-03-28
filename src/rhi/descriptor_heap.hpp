/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:29:53
 */

#pragma once

#include "device.hpp"

#include <vector>

enum class DescriptorHeapType
{
    RenderTarget = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
    DepthTarget = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
    ShaderResource = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    Sampler = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
};

class DescriptorHeap
{
public:
    using Ptr = std::shared_ptr<DescriptorHeap>;

    struct Descriptor
    {
        bool Valid;
        int HeapIndex;
        D3D12_CPU_DESCRIPTOR_HANDLE CPU;
        D3D12_GPU_DESCRIPTOR_HANDLE GPU;
        DescriptorHeap *ParentHeap;

        Descriptor();
        Descriptor(DescriptorHeap *parentHeap, int index);
    };

public:
    DescriptorHeap(Device::Ptr devicePtr, DescriptorHeapType type, uint32_t size);
    ~DescriptorHeap();

    Descriptor Allocate();
    void Free(Descriptor descriptor);

    ID3D12DescriptorHeap* GetHeap() { return _heap; }
private:
    Device::Ptr _devicePtr;
    ID3D12DescriptorHeap* _heap;
    D3D12_DESCRIPTOR_HEAP_TYPE _type;

    int _incrementSize;
    uint32_t _heapSize;

    std::vector<bool> _table;
};
