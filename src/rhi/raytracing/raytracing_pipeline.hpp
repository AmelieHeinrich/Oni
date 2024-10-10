//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 21:14:34
//

#pragma once

#include "rhi/texture.hpp"
#include "rhi/root_signature.hpp"
#include "rhi/buffer.hpp"

#include <core/shader_bytecode.hpp>

struct RaytracingPipelineSpecs
{
    uint32_t PayloadSize;
    uint32_t AttributeSize = 8;
    uint32_t MaxTraceRecursionDepth = 3;

    ShaderBytecode LibBytecode;
    RootSignature::Ptr Signature = nullptr;
};

class RaytracingPipeline
{
public:
    using Ptr = std::shared_ptr<RaytracingPipeline>;

    RaytracingPipeline(Device::Ptr device, Allocator::Ptr, DescriptorHeap::Heaps& heaps, RaytracingPipelineSpecs& specs);
    ~RaytracingPipeline();

    ID3D12StateObject* GetPipeline() { return _pipeline; }
    RootSignature::Ptr GetSignature() { return _signature; }
    Buffer::Ptr GetTables() { return _idBuffer; }
private:
    ID3D12StateObject* _pipeline = nullptr;
    RootSignature::Ptr _signature = nullptr;
    Buffer::Ptr _idBuffer = nullptr;
};
