/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:46:38
 */

#pragma once

#include "device.hpp"
#include "root_signature.hpp"

#include <core/shader_bytecode.hpp>

class ComputePipeline
{
public:
    using Ptr = std::shared_ptr<ComputePipeline>;

    ComputePipeline(Device::Ptr device, ShaderBytecode& bytecode, RootSignature::Ptr rootSignature = nullptr);
    ~ComputePipeline();

    ID3D12PipelineState* GetPipeline() { return _pipeline; }
    RootSignature::Ptr GetSignature() { return _signature; }
private:
    ID3D12PipelineState* _pipeline;
    RootSignature::Ptr _signature;
};
