//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 20:56:58
//

#include "tlas.hpp"

TLAS::TLAS(Device::Ptr device, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, Buffer::Ptr instanceBuffer, uint32_t numInstances, const std::string& name)
{
    _inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    _inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    _inputs.NumDescs = numInstances;
    _inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    _inputs.InstanceDescs = instanceBuffer->Address();

    _accelerationStructure = ASBuilder::Allocate(_inputs, &_updateScratchSize, "TLAS");

    _tlasUpdate = std::make_shared<Buffer>(device, allocator, heaps, _updateScratchSize, 0, BufferType::Storage, false, name);
}

void TLAS::FreeScratch()
{
    _accelerationStructure.Scratch->Resource->Release();
    _accelerationStructure.Scratch->Allocation->Release();
    _accelerationStructure.Scratch->ClearFromAllocationList();
}

TLAS::~TLAS()
{
    _accelerationStructure.AS->Resource->Release();
    _accelerationStructure.AS->Allocation->Release();
    _accelerationStructure.AS->ClearFromAllocationList();
}
