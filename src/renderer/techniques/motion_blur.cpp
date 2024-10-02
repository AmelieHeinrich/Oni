//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-28 17:57:06
//

#include "motion_blur.hpp"

MotionBlur::MotionBlur(RenderContext::Ptr context, Texture::Ptr output)
    : _blurPipeline(PipelineType::Compute), _context(context), _output(output)
{
    _blurPipeline.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(uint32_t) * 4
    };
    _blurPipeline.ReflectRootSignature(false);
    _blurPipeline.AddShaderWatch("shaders/MotionBlur/MotionBlurCompute.hlsl", "Main", ShaderType::Compute);
    _blurPipeline.Build(_context);

    _pointSampler = _context->CreateSampler(SamplerAddress::Border, SamplerFilter::Nearest, false, 0);
}

void MotionBlur::Render(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(commandBuffer->GetCommandList());
    OPTICK_GPU_EVENT("Motion Blur");

    struct Data {
        uint32_t Velocity;
        uint32_t Output;
        uint32_t PointSampler;
        uint32_t SampleCount;
    };
    Data data = {
        _velocityBuffer->SRV(),
        _output->UAV(),
        _pointSampler->BindlesssSampler(),
        _sampleCount
    };

    if (_enabled) {
        commandBuffer->BeginEvent("Motion Blur");
        commandBuffer->ImageBarrierBatch({
            { _velocityBuffer, TextureLayout::ShaderResource },
            { _output, TextureLayout::Storage }
        });
        commandBuffer->BindComputePipeline(_blurPipeline.ComputePipeline);
        commandBuffer->PushConstantsCompute(&data, sizeof(data), 0);
        commandBuffer->Dispatch(std::ceil(width / 8), std::ceil(height / 8), 1);
        commandBuffer->ImageBarrier(_output, TextureLayout::Storage);
        commandBuffer->EndEvent();
    }
}

void MotionBlur::Resize(uint32_t width, uint32_t height)
{
    // unused
}

void MotionBlur::OnUI()
{
    if (ImGui::TreeNodeEx("Motion Blur", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Enabled", &_enabled);
        ImGui::SliderInt("Sample Count", reinterpret_cast<int*>(&_sampleCount), 1, 8);
        ImGui::TreePop();
    }
}

void MotionBlur::Reconstruct()
{
    _blurPipeline.CheckForRebuild(_context, "Motion Blur");
}
