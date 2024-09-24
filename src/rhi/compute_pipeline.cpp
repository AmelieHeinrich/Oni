/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:47:47
 */

#include "compute_pipeline.hpp"

#include <dxcapi.h>
#include <d3d12shader.h>
#include <vector>
#include <array>
#include <algorithm>

#include <core/log.hpp>

ComputePipeline::ComputePipeline(Device::Ptr device, ShaderBytecode& bytecode, RootSignature::Ptr rootSignature)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.CS.pShaderBytecode = bytecode.bytecode.data();
    desc.CS.BytecodeLength = bytecode.bytecode.size() * sizeof(uint32_t);
    if (rootSignature) {
        desc.pRootSignature = rootSignature->GetSignature();
        _signature = rootSignature;
    } else {
        _signature = std::make_shared<RootSignature>(device);
        _signature->ReflectFromComputeShader(bytecode);
        desc.pRootSignature = _signature->GetSignature();
    }

    HRESULT Result = device->GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&_pipeline));
    if (FAILED(Result)) {
        Logger::Error("D3D12: Failed creating D3D12 compute pipeline!");
        return;
    }
}

ComputePipeline::~ComputePipeline()
{
    _pipeline->Release();
}
