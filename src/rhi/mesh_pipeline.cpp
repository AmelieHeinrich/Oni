//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-07 19:41:54
//

#include "mesh_pipeline.hpp"

#include "core/log.hpp"

MeshPipeline::MeshPipeline(Device::Ptr devicePtr, GraphicsPipelineSpecs& specs)
{
    ShaderBytecode& ampBytecode = specs.Bytecodes[ShaderType::Amplification];
    ShaderBytecode& meshBytecode = specs.Bytecodes[ShaderType::Mesh];
    ShaderBytecode& fragmentBytecode = specs.Bytecodes[ShaderType::Fragment];

    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC Desc = {};
    if (specs.UseAmplification) {
        Desc.AS.pShaderBytecode = ampBytecode.bytecode.data();
        Desc.AS.BytecodeLength = ampBytecode.bytecode.size() * sizeof(uint32_t);
    }
    Desc.MS.pShaderBytecode = meshBytecode.bytecode.data();
    Desc.MS.BytecodeLength = meshBytecode.bytecode.size() * sizeof(uint32_t);
    Desc.PS.pShaderBytecode = fragmentBytecode.bytecode.data();
    Desc.PS.BytecodeLength = fragmentBytecode.bytecode.size() * sizeof(uint32_t);
    for (int RTVIndex = 0; RTVIndex < specs.FormatCount; RTVIndex++) {
        Desc.BlendState.RenderTarget[RTVIndex].SrcBlend = D3D12_BLEND_ONE;
        Desc.BlendState.RenderTarget[RTVIndex].DestBlend = D3D12_BLEND_ZERO;
        Desc.BlendState.RenderTarget[RTVIndex].BlendOp = D3D12_BLEND_OP_ADD;
        Desc.BlendState.RenderTarget[RTVIndex].SrcBlendAlpha = D3D12_BLEND_ONE;
        Desc.BlendState.RenderTarget[RTVIndex].DestBlendAlpha = D3D12_BLEND_ZERO;
        Desc.BlendState.RenderTarget[RTVIndex].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        Desc.BlendState.RenderTarget[RTVIndex].LogicOp = D3D12_LOGIC_OP_NOOP;
        Desc.BlendState.RenderTarget[RTVIndex].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        Desc.RTVFormats[RTVIndex] = DXGI_FORMAT(specs.Formats[RTVIndex]);
        Desc.NumRenderTargets = specs.FormatCount;
    }
    Desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    Desc.RasterizerState.FillMode = D3D12_FILL_MODE(specs.Fill);
    Desc.RasterizerState.CullMode = D3D12_CULL_MODE(specs.Cull);
    Desc.RasterizerState.DepthClipEnable = specs.DepthClipEnable;
    Desc.RasterizerState.FrontCounterClockwise = specs.CCW;
    Desc.PrimitiveTopologyType = specs.Line ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    if (specs.DepthEnabled) {
        Desc.DepthStencilState.DepthEnable = true;
        Desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        Desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC(specs.Depth);
        Desc.DSVFormat = DXGI_FORMAT(specs.DepthFormat);
    }
    Desc.SampleDesc.Count = 1;
    if (specs.Signature) {
        Desc.pRootSignature = specs.Signature->GetSignature();
        _signature = specs.Signature;
    }

    auto Stream = CD3DX12_PIPELINE_MESH_STATE_STREAM(Desc);

    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
    streamDesc.pPipelineStateSubobjectStream = &Stream;
    streamDesc.SizeInBytes = sizeof(Stream);

    HRESULT Result = devicePtr->GetDevice()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&_pipeline));
    if (FAILED(Result)) {
        Logger::Error("[D3D12] Failed creating D3D12 graphics pipeline!");
        return;
    }
}

MeshPipeline::~MeshPipeline()
{
    _pipeline->Release();
}
