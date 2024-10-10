//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 19:04:05
//

#pragma once

#include "rhi/allocator.hpp"

struct AccelerationStructure
{
    GPUResource* AS;
    GPUResource* Scratch;
};

class ASBuilder
{
public:
    static void Init(Allocator::Ptr allocator, Device::Ptr device);

    static AccelerationStructure Allocate(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, uint64_t *scratchSize = nullptr, const std::string& name = "Acceleration Structure");
private:
    struct Data {
        Allocator::Ptr Allocator;
        Device::Ptr Device;
    };

    static Data _Data;
};
