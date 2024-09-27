//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-24 17:26:06
//

#pragma once

#include "device.hpp"
#include "core/shader_bytecode.hpp"

#include <array>
#include <dxcapi.h>
#include <d3d12shader.h>
#include <vector>
#include <array>
#include <algorithm>

enum class RootSignatureEntry
{
    PushConstants = 999,
    CBV = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
    SRV = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
    UAV = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
    Sampler = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER
};

struct RootSignatureBuildInfo
{
    std::vector<RootSignatureEntry> Entries;
    uint64_t PushConstantSize;
};

// WARNING: Root signature reflection does not support push constants

class RootSignature
{
public:
    using Ptr = std::shared_ptr<RootSignature>;

    RootSignature(Device::Ptr device);
    RootSignature(Device::Ptr device, const RootSignatureBuildInfo& buildInfo);
    ~RootSignature();

    void ReflectFromGraphicsShader(ShaderBytecode& vertexBytecode, ShaderBytecode& fragmentBytecode);
    void ReflectFromComputeShader(ShaderBytecode& computeBytecode);

    ID3D12RootSignature* GetSignature() { return _rootSignature; }

    static ID3D12ShaderReflection* GetReflection(ShaderBytecode& bytecode, D3D12_SHADER_DESC *desc);
private:
    Device::Ptr _device;

    ID3D12RootSignature* _rootSignature;
};