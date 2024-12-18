/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 15:41:54
 */

#pragma once

#include "texture.hpp"
#include "root_signature.hpp"

#include <core/shader_bytecode.hpp>
#include <unordered_map>

enum class FillMode
{
    Solid = D3D12_FILL_MODE_SOLID,
    Line = D3D12_FILL_MODE_WIREFRAME
};

enum class CullMode
{
    Back = D3D12_CULL_MODE_BACK,
    Front = D3D12_CULL_MODE_FRONT,
    None = D3D12_CULL_MODE_NONE
};

enum class DepthOperation
{
    Greater = D3D12_COMPARISON_FUNC_GREATER,
    Less = D3D12_COMPARISON_FUNC_LESS,
    Equal = D3D12_COMPARISON_FUNC_EQUAL,
    LEqual = D3D12_COMPARISON_FUNC_LESS_EQUAL,
    None = D3D12_COMPARISON_FUNC_NONE
};

struct GraphicsPipelineSpecs
{
    FillMode Fill;
    CullMode Cull;
    DepthOperation Depth;

    TextureFormat Formats[32];
    int FormatCount;
    TextureFormat DepthFormat;
    bool DepthEnabled;
    bool CCW = true;
    bool Line = false;
    bool DepthClipEnable = true;

    std::unordered_map<ShaderType, ShaderBytecode> Bytecodes;
    RootSignature::Ptr Signature = nullptr;

    bool UseAmplification = false;
};

class GraphicsPipeline
{
public:
    using Ptr = std::shared_ptr<GraphicsPipeline>;

    GraphicsPipeline(Device::Ptr devicePtr, GraphicsPipelineSpecs& specs);
    ~GraphicsPipeline();

    ID3D12PipelineState* GetPipeline() { return _pipeline; }
    RootSignature::Ptr GetSignature() { return _signature; }
private:
    ID3D12PipelineState* _pipeline;
    RootSignature::Ptr _signature;
};
