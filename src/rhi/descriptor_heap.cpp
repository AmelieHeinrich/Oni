/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:37:16
 */

#include "descriptor_heap.hpp"

#include <core/log.hpp>

const char* D3D12HeapTypeToStr(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
    switch (Type)
    {
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            return "D3D12_DESCRIPTOR_HEAP_TYPE_RTV";
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            return "D3D12_DESCRIPTOR_HEAP_TYPE_DSV";
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            return "D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV";
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            return "D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER";
    }
    return "?????";
}

DescriptorHeap::Descriptor::Descriptor()
    : ParentHeap(nullptr), HeapIndex(-1), Valid(false)
{
}

DescriptorHeap::Descriptor::Descriptor(DescriptorHeap *parentHeap, int index)
    : ParentHeap(parentHeap), HeapIndex(index)
{
    if (HeapIndex == -1) {
        Valid = false;
        return;
    }

    CPU = parentHeap->_heap->GetCPUDescriptorHandleForHeapStart();
    CPU.ptr += HeapIndex * parentHeap->_incrementSize;

    if (parentHeap->_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || parentHeap->_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
        GPU = parentHeap->_heap->GetGPUDescriptorHandleForHeapStart();
        GPU.ptr += HeapIndex * parentHeap->_incrementSize;
    }

    Valid = true;
}

DescriptorHeap::DescriptorHeap(Device::Ptr devicePtr, DescriptorHeapType type, uint32_t size)
    : _devicePtr(devicePtr), _type(D3D12_DESCRIPTOR_HEAP_TYPE(type)), _heapSize(size)
{
    _table = std::vector<bool>(size, false);

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = _type;
    desc.NumDescriptors = size;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || _type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT result = _devicePtr->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap));
    if (FAILED(result)) {
        Logger::Error("Failed to create descriptor heap!");
    }

    _incrementSize = _devicePtr->GetDevice()->GetDescriptorHandleIncrementSize(_type);

    Logger::Info("[D3D12] Allocated descriptor heap of type %s and size %u", D3D12HeapTypeToStr(_type), _heapSize);
}

DescriptorHeap::~DescriptorHeap()
{
    _table.clear();
    _heap->Release();
}

DescriptorHeap::Descriptor DescriptorHeap::Allocate()
{
    int index = -1;

    for (int i = 0; i < _heapSize; i++) {
        if (_table[i] == false) {
            _table[i] = true;
            index = i;
            break;
        }
    }
    if (index == -1) {
        Logger::Error("Failed to find suitable descriptor!");
        return Descriptor(this, -1);
    } else {
        return Descriptor(this, index);
    }
}

void DescriptorHeap::Free(DescriptorHeap::Descriptor descriptor)
{
    if (!descriptor.Valid) {
        return;
    }
    _table[descriptor.HeapIndex] = false;
    descriptor.Valid = false;
    descriptor.ParentHeap = nullptr;
}
