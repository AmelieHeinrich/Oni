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

extern bool CompareShaderInput(const D3D12_SHADER_INPUT_BIND_DESC& A, const D3D12_SHADER_INPUT_BIND_DESC& B);

ComputePipeline::ComputePipeline(Device::Ptr device, ShaderBytecode& bytecode)
{
    D3D12_SHADER_DESC ComputeDesc = {};
    ID3D12ShaderReflection* pReflection;

    {
        IDxcUtils* pUtils;
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));

        DxcBuffer ShaderBuffer = {};
        ShaderBuffer.Ptr = bytecode.bytecode.data();
        ShaderBuffer.Size = bytecode.bytecode.size() * sizeof(uint32_t);
        HRESULT Result = pUtils->CreateReflection(&ShaderBuffer, IID_PPV_ARGS(&pReflection));
        if (FAILED(Result)) {
            Logger::Error("Failed to get reflection from shader");
        }
        pReflection->GetDesc(&ComputeDesc);
        pUtils->Release();
    }

    std::array<D3D12_ROOT_PARAMETER, 64> Parameters;
    int ParameterCount = 0;

    std::array<D3D12_DESCRIPTOR_RANGE, 64> Ranges;
    int RangeCount = 0;

    std::array<D3D12_SHADER_INPUT_BIND_DESC, 64> ShaderBinds;
    int BindCount = 0;

    for (int BoundResourceIndex = 0; BoundResourceIndex < ComputeDesc.BoundResources; BoundResourceIndex++) {
        D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = {};
        pReflection->GetResourceBindingDesc(BoundResourceIndex, &ShaderInputBindDesc);
        ShaderBinds[BindCount] = ShaderInputBindDesc;
        BindCount++;
    }

    std::sort(ShaderBinds.begin(), ShaderBinds.begin() + BindCount, CompareShaderInput);

    for (int ShaderBindIndex = 0; ShaderBindIndex < BindCount; ShaderBindIndex++) {
        D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = ShaderBinds[ShaderBindIndex];

        D3D12_ROOT_PARAMETER RootParameter = {};
        RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        D3D12_DESCRIPTOR_RANGE Range = {};
        Range.NumDescriptors = 1;
        Range.BaseShaderRegister = ShaderInputBindDesc.BindPoint;

        switch (ShaderInputBindDesc.Type) {
            case D3D_SIT_SAMPLER:
                Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                break;
            case D3D_SIT_TEXTURE:
                Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                break;
            case D3D_SIT_UAV_RWTYPED:
                Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                break;
            case D3D_SIT_CBUFFER:
                Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                break;
            case D3D_SIT_UAV_RWBYTEADDRESS:
                Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                break;
        }

        Ranges[RangeCount] = Range;

        RootParameter.DescriptorTable.NumDescriptorRanges = 1;
        RootParameter.DescriptorTable.pDescriptorRanges = &Ranges[RangeCount];
        RootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        Parameters[ParameterCount] = RootParameter;

        ParameterCount++;
        RangeCount++;
    }

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
    RootSignatureDesc.NumParameters = ParameterCount;
    RootSignatureDesc.pParameters = Parameters.data();
    
    ID3DBlob* pRootSignatureBlob;
    ID3DBlob* pErrorBlob;
    D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pRootSignatureBlob, &pErrorBlob);
    if (pErrorBlob) {
        Logger::Error("D3D12 Root Signature error! %s", pErrorBlob->GetBufferPointer());
    }
    HRESULT Result = device->GetDevice()->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
    if (FAILED(Result)) {
        Logger::Error("Failed to create root signature!");
    }
    pRootSignatureBlob->Release();

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = _rootSignature;
    desc.CS.pShaderBytecode = bytecode.bytecode.data();
    desc.CS.BytecodeLength = bytecode.bytecode.size() * sizeof(uint32_t);
    
    Result = device->GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&_pipeline));
    if (FAILED(Result)) {
        Logger::Error("D3D12: Failed creating D3D12 compute pipeline!");
        return;
    }
    pReflection->Release();
}

ComputePipeline::~ComputePipeline()
{
    _rootSignature->Release();
    _pipeline->Release();
}
