/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 15:47:47
 */

#include "graphics_pipeline.hpp"

#include "core/log.hpp"

GraphicsPipeline::GraphicsPipeline(Device::Ptr devicePtr, GraphicsPipelineSpecs& specs)
{
    ShaderBytecode& vertexBytecode = specs.Bytecodes[ShaderType::Vertex];
    ShaderBytecode& fragmentBytecode = specs.Bytecodes[ShaderType::Fragment];

    D3D12_SHADER_DESC VertexDesc = {};
    ID3D12ShaderReflection* pVertexReflection = RootSignature::GetReflection(vertexBytecode, &VertexDesc);

    std::vector<D3D12_INPUT_ELEMENT_DESC> InputElementDescs;
    std::vector<std::string> InputElementSemanticNames;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {};
    Desc.VS.pShaderBytecode = vertexBytecode.bytecode.data();
    Desc.VS.BytecodeLength = vertexBytecode.bytecode.size() * sizeof(uint32_t);
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

    InputElementSemanticNames.reserve(VertexDesc.InputParameters);
    InputElementDescs.reserve(VertexDesc.InputParameters);

    for (int ParameterIndex = 0; ParameterIndex < VertexDesc.InputParameters; ParameterIndex++)
    {
        D3D12_SIGNATURE_PARAMETER_DESC ParameterDesc = {};
        pVertexReflection->GetInputParameterDesc(ParameterIndex, &ParameterDesc);

        InputElementSemanticNames.push_back(ParameterDesc.SemanticName);

        D3D12_INPUT_ELEMENT_DESC InputElement = {};
        InputElement.SemanticName = InputElementSemanticNames.back().c_str();
        InputElement.SemanticIndex = ParameterDesc.SemanticIndex;
        InputElement.InputSlot = 0;
        InputElement.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        InputElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        InputElement.InstanceDataStepRate = 0;

        if (ParameterDesc.Mask == 1)
        {
            if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) InputElement.Format = DXGI_FORMAT_R32_UINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) InputElement.Format = DXGI_FORMAT_R32_SINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputElement.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (ParameterDesc.Mask <= 3)
        {
            if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) InputElement.Format = DXGI_FORMAT_R32G32_UINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) InputElement.Format = DXGI_FORMAT_R32G32_SINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputElement.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (ParameterDesc.Mask <= 7)
        {
            if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) InputElement.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) InputElement.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputElement.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (ParameterDesc.Mask <= 15)
        {
            if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) InputElement.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) InputElement.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputElement.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        InputElementDescs.push_back(InputElement);
    }
    Desc.InputLayout.pInputElementDescs = InputElementDescs.data();
    Desc.InputLayout.NumElements = static_cast<uint32_t>(InputElementDescs.size());
    if (specs.Signature) {
        Desc.pRootSignature = specs.Signature->GetSignature();
        _signature = specs.Signature;
    }

    HRESULT Result = devicePtr->GetDevice()->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(&_pipeline));
    if (FAILED(Result)) {
        Logger::Error("D3D12: Failed creating D3D12 graphics pipeline!");
        return;
    }
}

GraphicsPipeline::~GraphicsPipeline()
{
    _pipeline->Release();
}
