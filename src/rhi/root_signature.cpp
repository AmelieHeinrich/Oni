//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-24 17:31:56
//

#include "core/log.hpp"
#include "root_signature.hpp"

ID3D12ShaderReflection* RootSignature::GetReflection(ShaderBytecode& bytecode, D3D12_SHADER_DESC *desc)
{
    IDxcUtils* pUtils;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
    
    DxcBuffer ShaderBuffer = {};
    ShaderBuffer.Ptr = bytecode.bytecode.data();
    ShaderBuffer.Size = bytecode.bytecode.size() * sizeof(uint32_t);
    ID3D12ShaderReflection* pReflection;
    HRESULT Result = pUtils->CreateReflection(&ShaderBuffer, IID_PPV_ARGS(&pReflection));
    if (FAILED(Result)) {
        Logger::Error("Failed to get reflection from shader");
    }
    pReflection->GetDesc(desc);
    pUtils->Release();
    return pReflection;
}

bool CompareShaderInput(const D3D12_SHADER_INPUT_BIND_DESC& A, const D3D12_SHADER_INPUT_BIND_DESC& B)
{
    return A.BindPoint < B.BindPoint;
}

RootSignature::RootSignature(Device::Ptr device)
    : _device(device)
{
    // Do nothing, empty constructor means client will call ReflectFromXXX.
}

RootSignature::RootSignature(Device::Ptr device, const RootSignatureBuildInfo& buildInfo)
    : _device(device)
{
    std::vector<D3D12_ROOT_PARAMETER> Parameters(buildInfo.Entries.size());
    std::vector<D3D12_DESCRIPTOR_RANGE> Ranges(buildInfo.Entries.size());
    
    for (int i = 0; i < buildInfo.Entries.size(); i++) {
        D3D12_DESCRIPTOR_RANGE& descriptorRange = Ranges[i];
        descriptorRange.NumDescriptors = 1;
        descriptorRange.BaseShaderRegister = i;
        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE(buildInfo.Entries[i]);
        descriptorRange.RegisterSpace = 0;
        
        D3D12_ROOT_PARAMETER& parameter = Parameters[i];
        if (buildInfo.Entries[i] == RootSignatureEntry::PushConstants) {
            parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            parameter.Constants.ShaderRegister = i;
            parameter.Constants.RegisterSpace = 0;
            parameter.Constants.Num32BitValues = buildInfo.PushConstantSize / 4;
        } else {
            parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            parameter.DescriptorTable.NumDescriptorRanges = 1;
            parameter.DescriptorTable.pDescriptorRanges = &descriptorRange;
            parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
    }

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
    RootSignatureDesc.NumParameters = Parameters.size();
    RootSignatureDesc.pParameters = Parameters.data();
    RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                            | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
                            | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

    ID3DBlob* pRootSignatureBlob;
    ID3DBlob* pErrorBlob;
    D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pRootSignatureBlob, &pErrorBlob);
    if (pErrorBlob) {
        Logger::Error("D3D12 Root Signature error! %s", pErrorBlob->GetBufferPointer());
    }
    HRESULT Result = _device->GetDevice()->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
    if (FAILED(Result)) {
        Logger::Error("Failed to create root signature!");
    }
    pRootSignatureBlob->Release();
}

RootSignature::~RootSignature()
{
    _rootSignature->Release();
}

void RootSignature::ReflectFromGraphicsShader(ShaderBytecode& vertexBytecode, ShaderBytecode& fragmentBytecode)
{
    D3D12_SHADER_DESC VertexDesc = {};
    D3D12_SHADER_DESC PixelDesc = {};

    std::array<D3D12_ROOT_PARAMETER, 64> Parameters = {};
    int ParameterCount = 0;

    std::array<D3D12_DESCRIPTOR_RANGE, 64> Ranges = {};
    int RangeCount = 0;

    std::array<D3D12_SHADER_INPUT_BIND_DESC, 64> ShaderBinds = {};
    int BindCount = 0;

    ID3D12ShaderReflection* pVertexReflection = GetReflection(vertexBytecode, &VertexDesc);
    ID3D12ShaderReflection* pPixelReflection = GetReflection(fragmentBytecode, &PixelDesc);

    for (int BoundResourceIndex = 0; BoundResourceIndex < VertexDesc.BoundResources; BoundResourceIndex++) {
        D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = {};
        pVertexReflection->GetResourceBindingDesc(BoundResourceIndex, &ShaderInputBindDesc);
        ShaderBinds[BindCount] = ShaderInputBindDesc;
        BindCount++;
    }

    for (int BoundResourceIndex = 0; BoundResourceIndex < PixelDesc.BoundResources; BoundResourceIndex++) {
        D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = {};
        pPixelReflection->GetResourceBindingDesc(BoundResourceIndex, &ShaderInputBindDesc);
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
    RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* pRootSignatureBlob;
    ID3DBlob* pErrorBlob;
    D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pRootSignatureBlob, &pErrorBlob);
    if (pErrorBlob) {
        Logger::Error("D3D12 Root Signature error! %s", pErrorBlob->GetBufferPointer());
    }
    HRESULT Result = _device->GetDevice()->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
    if (FAILED(Result)) {
        Logger::Error("Failed to create root signature!");
    }
    pRootSignatureBlob->Release();

    pPixelReflection->Release();
    pVertexReflection->Release();
}

void RootSignature::ReflectFromComputeShader(ShaderBytecode& computeBytecode)
{
    D3D12_SHADER_DESC ComputeDesc = {};
    ID3D12ShaderReflection* pReflection = GetReflection(computeBytecode, &ComputeDesc);

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
    HRESULT Result = _device->GetDevice()->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
    if (FAILED(Result)) {
        Logger::Error("Failed to create root signature!");
    }
    pRootSignatureBlob->Release();

    pReflection->Release();
}
