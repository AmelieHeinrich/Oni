/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:43:16
 */

#pragma once

#include "device.hpp"

#include "D3D12MA/D3D12MemAlloc.h"

#include <string>
#include <vector>

class Texture;
class Allocator;

enum class GPUResourceType
{
    Buffer,
    Texture,
    CubeMap
};

struct GPUResource
{
    ID3D12Resource* Resource;
    D3D12MA::Allocation* Allocation;
    GPUResourceType Type;
    std::string Name;
    uint64_t Size;

    Texture* AttachedTexture;
    Allocator* ParentAllocator;
    void AttachTexture(Texture* texture) { AttachedTexture = texture; }

    void ClearFromAllocationList();
};

class Allocator
{
public:
    struct Stats
    {
        uint64_t Total;
        uint64_t Used;
    };

    using Ptr = std::shared_ptr<Allocator>;

    Allocator(Device::Ptr devicePtr);
    ~Allocator();

    GPUResource *Allocate(D3D12MA::ALLOCATION_DESC *allocDesc, D3D12_RESOURCE_DESC *resourceDesc, D3D12_RESOURCE_STATES state, const std::string& name = "Allocation");

    D3D12MA::Allocator* GetAllocator() { return _allocator; }

    void OnGUI();

    Stats GetStats();
private:
    friend struct GPUResource;

    D3D12MA::Allocator* _allocator;
    std::vector<GPUResource*> _allocations;
    GPUResource* _uiSelected;
};
