//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-24 23:30:09
//

#include "chromatic_aberration.hpp"

ChromaticAberration::ChromaticAberration(RenderContext::Ptr context, Texture::Ptr input)
    : _renderContext(context), _inputHDR(input), _computePipeline(PipelineType::Compute)
{
    _computePipeline.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(glm::uvec4)
    };
    _computePipeline.ReflectRootSignature(false);
    _computePipeline.AddShaderWatch("shaders/ChromaticAberration/ChromaticAberrationCompute.hlsl", "Main", ShaderType::Compute);
    _computePipeline.Build(context);
}

void ChromaticAberration::Render(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr cmdBuf = _renderContext->GetCurrentCommandBuffer();

    struct PushConstants {
        uint32_t Input;
        glm::ivec3 Offsets;
    };
    PushConstants constants = {
        _inputHDR->SRV(),
        _offsetInPixels
    };

    OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
    OPTICK_GPU_EVENT("Chromatic Aberration");

    if (_enable) {
        cmdBuf->BeginEvent("Chromatic Aberration");
        cmdBuf->ImageBarrier(_inputHDR, TextureLayout::Storage);
        cmdBuf->BindComputePipeline(_computePipeline.ComputePipeline);
        cmdBuf->PushConstantsCompute(&constants, sizeof(PushConstants), 0);
        cmdBuf->Dispatch(std::ceil(width / 8), std::ceil(height / 8), 1);
        cmdBuf->EndEvent();
    }
}

void ChromaticAberration::Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR)
{
    _inputHDR = inputHDR;
}

void ChromaticAberration::OnUI()
{
    if (ImGui::TreeNodeEx("Chromatic Aberration", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Enable", &_enable);
        ImGui::SliderInt3("Offsets", glm::value_ptr(_offsetInPixels), 0, 20);
        ImGui::TreePop();
    }
}

void ChromaticAberration::Reconstruct()
{
    _computePipeline.CheckForRebuild(_renderContext, "Chromatic Aberration");
}
