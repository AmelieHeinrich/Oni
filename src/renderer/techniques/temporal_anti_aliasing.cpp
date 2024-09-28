//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-27 17:35:10
//

#include "temporal_anti_aliasing.hpp"

TemporalAntiAliasing::TemporalAntiAliasing(RenderContext::Ptr renderContext, Texture::Ptr output)
    : _context(renderContext), _output(output), _taaPipeline(PipelineType::Compute)
{
    _history = renderContext->CreateTexture(output->GetWidth(), output->GetHeight(), output->GetFormat(), TextureUsage::ShaderResource, false, "[TAA] History Texture");
    _history->BuildShaderResource();
    _history->BuildStorage();

    _taaPipeline.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(uint32_t) * 5
    };
    _taaPipeline.ReflectRootSignature(false);
    _taaPipeline.AddShaderWatch("shaders/TAA/TAACompute.hlsl", "Main", ShaderType::Compute);
    _taaPipeline.Build(_context);

    _linearSampler = _context->CreateSampler(SamplerAddress::Border, SamplerFilter::Linear, false, 0);
    _pointSampler = _context->CreateSampler(SamplerAddress::Border, SamplerFilter::Nearest, false, 0);
}

void TemporalAntiAliasing::Render(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(commandBuffer->GetCommandList());
    OPTICK_GPU_EVENT("Temporal Anti Aliasing");

    if (_enabled) {
        commandBuffer->BeginEvent("Temporal Anti Aliasing");
        AccumulateHistory(width, height);
        Resolve(width, height);
        commandBuffer->EndEvent();
    }
}

void TemporalAntiAliasing::AccumulateHistory(uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(commandBuffer->GetCommandList());
    OPTICK_GPU_EVENT("TAA Accumulate History");

    struct Data {
        uint32_t History;
        uint32_t Current;
        uint32_t Velocity;
        uint32_t LinearSampler;
        uint32_t PointSampler;
    };
    Data data;
    data.History = _history->SRV();
    data.Current = _output->UAV();
    data.Velocity = _velocityBuffer->SRV();
    data.LinearSampler = _linearSampler->BindlesssSampler();
    data.PointSampler = _pointSampler->BindlesssSampler();

    commandBuffer->BeginEvent("Resolve");
    commandBuffer->BindComputePipeline(_taaPipeline.ComputePipeline);
    commandBuffer->PushConstantsCompute(&data, sizeof(data), 0);
    commandBuffer->ImageBarrierBatch({
        { _history, TextureLayout::ShaderResource },
        { _output, TextureLayout::Storage },
        { _velocityBuffer, TextureLayout::ShaderResource }
    });
    commandBuffer->Dispatch(width / 8, height / 8, 1);
    commandBuffer->EndEvent();
}

void TemporalAntiAliasing::Resolve(uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(commandBuffer->GetCommandList());
    OPTICK_GPU_EVENT("TAA Copy to History");

    commandBuffer->BeginEvent("TAA Copy to History");
    commandBuffer->ImageBarrierBatch({
        { _history, TextureLayout::CopyDest },
        { _output, TextureLayout::CopySource },
    });
    commandBuffer->CopyTextureToTexture(_history, _output);
    commandBuffer->ImageBarrierBatch({
        { _history, TextureLayout::ShaderResource },
        { _output, TextureLayout::Storage },
    });
    commandBuffer->EndEvent();
}

void TemporalAntiAliasing::Resize(uint32_t width, uint32_t height)
{

}

void TemporalAntiAliasing::OnUI()
{
    if (ImGui::TreeNodeEx("Temporal Anti Aliasing", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Enable", &_enabled);
        ImGui::SliderFloat("Modulation Factor", &_modulationFactor, 0.0f, 1.0f, "%.1f");
        ImGui::TreePop();
    }
}

void TemporalAntiAliasing::Reconstruct()
{
    _taaPipeline.CheckForRebuild(_context, "TAA");
}
