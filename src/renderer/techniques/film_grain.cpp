//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-28 22:20:53
//

#include "film_grain.hpp"

FilmGrain::FilmGrain(RenderContext::Ptr context, Texture::Ptr output)
    : _renderContext(context), _inputHDR(output), _computePipeline(PipelineType::Compute)
{
    _computePipeline.SignatureInfo.Entries = {
        RootSignatureEntry::PushConstants
    };
    _computePipeline.SignatureInfo.PushConstantSize = sizeof(float) * 3;

    _computePipeline.ReflectRootSignature(false);
    _computePipeline.AddShaderWatch("shaders/FilmGrain/FilmGrainCompute.hlsl", "Main", ShaderType::Compute);
    _computePipeline.Build(context);
}

void FilmGrain::Render(Scene& scene, uint32_t width, uint32_t height, float dt)
{
    if (_enable) {
        CommandBuffer::Ptr cmdBuf = _renderContext->GetCurrentCommandBuffer();

        OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
        OPTICK_GPU_EVENT("Color Correction Pass");

        // Calculate parameters
        struct Data {
            uint32_t Texture;
            float Amount;
            float FrameTime;
        };
        Data data = {
            _inputHDR->UAV(),
            _amount,
            dt
        };

        cmdBuf->BeginEvent("Color Correction Pass");
        cmdBuf->ImageBarrier(_inputHDR, TextureLayout::Storage);
        cmdBuf->BindComputePipeline(_computePipeline.ComputePipeline);
        cmdBuf->PushConstantsCompute(&data, sizeof(data), 0);
        cmdBuf->Dispatch(std::ceil(width / 8), std::ceil(height / 8), 1);
        cmdBuf->ImageBarrier(_inputHDR, TextureLayout::RenderTarget);
        cmdBuf->EndEvent();
    }
}

void FilmGrain::Resize(uint32_t width, uint32_t height, Texture::Ptr output)
{
    
}

void FilmGrain::OnUI()
{
    if (ImGui::TreeNodeEx("Film Grain", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Enabled", &_enable);
        ImGui::SliderFloat("Strength", &_amount, 0.0f, 1.0f, "%.2f");
        ImGui::TreePop();
    }
}

void FilmGrain::Reconstruct()
{
    _computePipeline.CheckForRebuild(_renderContext, "Film Grain");
}
