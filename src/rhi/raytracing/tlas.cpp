//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 20:56:58
//

#include "tlas.hpp"

#include "core/log.hpp"

TLAS::TLAS(Device::Ptr device, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, Buffer::Ptr instanceBuffer, uint32_t numInstances, const std::string& name)
    : _heaps(heaps)
{
    _inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    _inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    _inputs.NumDescs = numInstances;
    _inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    _inputs.InstanceDescs = instanceBuffer->Address();

    _accelerationStructure = ASBuilder::Allocate(_inputs, &_updateScratchSize, "TLAS");

    _tlasUpdate = std::make_shared<Buffer>(device, allocator, heaps, _updateScratchSize, 0, BufferType::Storage, false, name);

    _srv = heaps.ShaderHeap->Allocate();

    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.RaytracingAccelerationStructure.Location = _accelerationStructure.AS->GetGPUVirtualAddress();

    device->GetDevice()->CreateShaderResourceView(nullptr, &desc, _srv.CPU);
}

void TLAS::FreeScratch()
{
    _accelerationStructure.Scratch->Release();
}

TLAS::~TLAS()
{
    _heaps.ShaderHeap->Free(_srv);

    _accelerationStructure.AS->Release();
}
