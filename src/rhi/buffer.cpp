/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 23:48:14
 */

#include "buffer.hpp"

#include <core/log.hpp>

Buffer::Buffer(Device::Ptr devicePtr, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, uint64_t size, uint64_t stride, BufferType type, bool readback, const std::string& name)
    : _size(size), _heaps(heaps), _devicePtr(devicePtr)
{
    D3D12MA::ALLOCATION_DESC AllocationDesc = {};
    AllocationDesc.HeapType = readback == true ? D3D12_HEAP_TYPE_READBACK : ((type == BufferType::Constant || type == BufferType::Copy) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    ResourceDesc.Width = size;
    ResourceDesc.Height = 1;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    if (type == BufferType::Storage) {
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    _state = type == BufferType::Constant ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;

    _resource = allocator->Allocate(&AllocationDesc, &ResourceDesc, _state, name);

    switch (type) {
        case BufferType::Vertex: {
            _VBV.BufferLocation = _resource->Resource->GetGPUVirtualAddress();
            _VBV.SizeInBytes = size;
            _VBV.StrideInBytes = stride;
            break;
        }
        case BufferType::Index: {
            _IBV.BufferLocation = _resource->Resource->GetGPUVirtualAddress();
            _IBV.SizeInBytes = size;
            _IBV.Format = DXGI_FORMAT_R32_UINT;
            break;
        }
        default: {
            break;
        }
    }
}

Buffer::~Buffer()
{
    if (_cbv.Valid) {
        _heaps.ShaderHeap->Free(_cbv);
    }
    if (_cbv.Valid) {
        _heaps.ShaderHeap->Free(_uav);
    }
    _resource->Allocation->Release();
    _resource->ClearFromAllocationList();
    delete _resource;
}

void Buffer::BuildConstantBuffer()
{
    _CBVD.BufferLocation = _resource->Resource->GetGPUVirtualAddress();
    _CBVD.SizeInBytes = _size;
    if (_cbv.Valid == false)
        _cbv = _heaps.ShaderHeap->Allocate();
    _devicePtr->GetDevice()->CreateConstantBufferView(&_CBVD, _cbv.CPU);
}

void Buffer::BuildStorage()
{
    _UAVD.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    _UAVD.Format = DXGI_FORMAT_UNKNOWN;
    _UAVD.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    _UAVD.Buffer.FirstElement = 0;
    _UAVD.Buffer.NumElements = _size / 4;
    _UAVD.Buffer.StructureByteStride = 4;
    _UAVD.Buffer.CounterOffsetInBytes = 0;
    if (_uav.Valid == false)
        _uav = _heaps.ShaderHeap->Allocate();
    _devicePtr->GetDevice()->CreateUnorderedAccessView(_resource->Resource, nullptr, &_UAVD, _uav.CPU);
}

void Buffer::Map(int start, int end, void **data)
{
    D3D12_RANGE range;
    range.Begin = start;
    range.End = end;

    HRESULT result = 0;
    if (range.End > range.Begin) {
        result = _resource->Resource->Map(0, &range, data);
    } else {
        result = _resource->Resource->Map(0, nullptr, data);
    }

    if (FAILED(result)) {
        Logger::Error("Failed to map buffer from range [%d-%d]", start, end);
    }
}

void Buffer::Unmap(int start, int end)
{
    D3D12_RANGE range;
    range.Begin = start;
    range.End = end;

    if (range.End > range.Begin) {
        _resource->Resource->Unmap(0, &range);
    } else {
        _resource->Resource->Unmap(0, nullptr);
    }
}