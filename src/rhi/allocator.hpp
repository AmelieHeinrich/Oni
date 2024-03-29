/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:43:16
 */

#pragma once

#include "device.hpp"

#include "D3D12MA/D3D12MemAlloc.h"

struct GPUResource
{
    ID3D12Resource* Resource;
    D3D12MA::Allocation* Allocation;
};

class Allocator
{
public:
    using Ptr = std::shared_ptr<Allocator>;

    Allocator(Device::Ptr devicePtr);
    ~Allocator();

    GPUResource Allocate(D3D12MA::ALLOCATION_DESC *allocDesc, D3D12_RESOURCE_DESC *resourceDesc, D3D12_RESOURCE_STATES state);

    D3D12MA::Allocator* GetAllocator() { return _allocator; }
private:
    D3D12MA::Allocator* _allocator;
};
