/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 17:24:42
 */

#include "color_correction.hpp"

#include <ImGui/imgui.h>
#include <optick.h>

ColorCorrection::ColorCorrection(RenderContext::Ptr context, Texture::Ptr inputHDR)
    : _renderContext(context), _inputHDR(inputHDR)
{
    ShaderBytecode bytecode;
    ShaderCompiler::CompileShader("shaders/ColorCorrection/ColorCorrectionCompute.hlsl", "Main", ShaderType::Compute, bytecode);
    _computePipeline = _renderContext->CreateComputePipeline(bytecode);

    _correctionParameters = context->CreateBuffer(256, 0, BufferType::Constant, false, "Color Correction Settings CBV");
    _correctionParameters->BuildConstantBuffer();
}

ColorCorrection::~ColorCorrection()
{
}

void ColorCorrection::Render(Scene& scene, uint32_t width, uint32_t height)
{
    if (_enable) {
        CommandBuffer::Ptr cmdBuf = _renderContext->GetCurrentCommandBuffer();

        OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
        OPTICK_GPU_EVENT("Color Correction Pass");

        void *pData;
        _correctionParameters->Map(0, 0, &pData);
        memcpy(pData, &_settings, sizeof(ColorCorrectionSettings));
        _correctionParameters->Unmap(0, 0);

        cmdBuf->BeginEvent("Color Correction Pass");
        cmdBuf->ImageBarrier(_inputHDR, TextureLayout::Storage);
        cmdBuf->BindComputePipeline(_computePipeline);
        cmdBuf->BindComputeShaderResource(_inputHDR, 0, 0);
        cmdBuf->BindComputeConstantBuffer(_correctionParameters, 1);
        cmdBuf->Dispatch(width / 30, height / 30, 1);
        cmdBuf->ImageBarrier(_inputHDR, TextureLayout::RenderTarget);
        cmdBuf->EndEvent();
    }
}

void ColorCorrection::Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR)
{
    _inputHDR = inputHDR;
}

void ColorCorrection::OnUI()
{
    if (ImGui::TreeNodeEx("Color Grading", ImGuiTreeNodeFlags_Framed))
    {
        ImGui::Checkbox("Enable", &_enable);
        ImGui::Separator();

        ImGui::SliderFloat("Exposure", &_settings.Exposure, 0.0f, 10.0f, "%.1f");
        ImGui::SliderFloat("Contrast", &_settings.Contrast, -100.0f, 100.0f, "%.1f");
        if (ImGui::TreeNodeEx("Color Filter")) {
            ImGui::ColorPicker3("Color Filter", glm::value_ptr(_settings.ColorFilter), ImGuiColorEditFlags_PickerHueBar);
            ImGui::TreePop();
        }
        ImGui::SliderFloat("Hue Shift", &_settings.HueShift, -180.0f, 180.0f, "%.1f");
        ImGui::SliderFloat("Saturation", &_settings.Saturation, -100.0f, 100.0f, "%.1f");

        ImGui::TreePop();
    }
}
