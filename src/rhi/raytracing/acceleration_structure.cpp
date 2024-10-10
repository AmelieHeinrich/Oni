//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 19:12:30
//

#include "acceleration_structure.hpp"

#include <cassert>

ASBuilder::Data ASBuilder::_Data;

void ASBuilder::Init(Allocator::Ptr allocator, Device::Ptr device)
{
    _Data.Allocator = allocator;
    _Data.Device = device;
}

AccelerationStructure ASBuilder::Allocate(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, uint64_t *scratchSize, const std::string& name)
{
    auto MakeBuffer = [](uint64_t Size, uint64_t alignment, D3D12_RESOURCE_STATES InitialState, const std::string& name) {
        D3D12_HEAP_PROPERTIES DEFAULT_HEAP = {};
        DEFAULT_HEAP.Type = D3D12_HEAP_TYPE_DEFAULT;
        
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = Size;
        desc.Height = 1,
        desc.DepthOrArraySize = 1,
        desc.MipLevels = 1,
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;
        desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

        ID3D12Resource* buffer;
        _Data.Device->GetDevice()->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, InitialState, nullptr, IID_PPV_ARGS(&buffer));
        
        std::wstring newName(name.begin(), name.end());
        buffer->SetName(newName.c_str());
        
        return buffer;
    };

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    _Data.Device->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    if (scratchSize)
        *scratchSize = prebuildInfo.UpdateScratchDataSizeInBytes;

    ID3D12Resource* scratch = MakeBuffer(prebuildInfo.ScratchDataSizeInBytes, 0, D3D12_RESOURCE_STATE_COMMON, "Scratch Buffer");
    ID3D12Resource* as = MakeBuffer(prebuildInfo.ResultDataMaxSizeInBytes, 0, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, name);

    AccelerationStructure structure;
    structure.Scratch = scratch;
    structure.AS = as;
    return structure;
}
