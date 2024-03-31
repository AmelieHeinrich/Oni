/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:46:38
 */

#pragma once

#include "device.hpp"

#include <shader/bytecode.hpp>

class ComputePipeline
{
public:
    using Ptr = std::shared_ptr<ComputePipeline>;

    ComputePipeline(Device::Ptr device, ShaderBytecode& bytecode);
    ~ComputePipeline();

    ID3D12PipelineState* GetPipeline() { return _pipeline; }
    ID3D12RootSignature* GetRootSignature() { return _rootSignature; }
private:
    ID3D12PipelineState* _pipeline;
    ID3D12RootSignature* _rootSignature;
};
