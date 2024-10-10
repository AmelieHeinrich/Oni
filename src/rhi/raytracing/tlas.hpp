//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 17:44:46
//

#pragma once

#include <glm/glm.hpp>

#include "rhi/device.hpp"
#include "rhi/buffer.hpp"
#include "rhi/allocator.hpp"

#include "acceleration_structure.hpp"

class TLAS
{
public:
    using Ptr = std::shared_ptr<TLAS>;

    TLAS(Device::Ptr device, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, Buffer::Ptr instanceBuffer, uint32_t numInstances, const std::string& name = "TLAS");
    ~TLAS();

    uint32_t SRV() { return _srv.HeapIndex; }
    void FreeScratch();
private:
    friend class RenderContext;
    DescriptorHeap::Heaps _heaps;

    AccelerationStructure _accelerationStructure = {};
    
    uint64_t _updateScratchSize = 0;
    Buffer::Ptr _tlasUpdate;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS _inputs = {};
    DescriptorHeap::Descriptor _srv;
};
