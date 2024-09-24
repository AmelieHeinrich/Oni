/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 17:24:42
 */

#include "color_correction.hpp"

#include <ImGui/imgui.h>
#include <optick.h>

ColorCorrection::ColorCorrection(RenderContext::Ptr context, Texture::Ptr inputHDR)
    : _renderContext(context), _inputHDR(inputHDR), _computePipeline(PipelineType::Compute)
{
    _computePipeline.SignatureInfo.Entries = {
        RootSignatureEntry::PushConstants
    };
    _computePipeline.SignatureInfo.PushConstantSize = sizeof(_settings);

    _computePipeline.ReflectRootSignature(false);
    _computePipeline.AddShaderWatch("shaders/ColorCorrection/ColorCorrectionCompute.hlsl", "Main", ShaderType::Compute);
    _computePipeline.Build(context);
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

        // Calculate parameters
        glm::vec4 splitColor = _settings.Shadows;
        splitColor.a = _settings.Balance * 0.01f;
        _settings.Shadows = splitColor;

        _settings.InputHDR = _inputHDR->UAV();

        cmdBuf->BeginEvent("Color Correction Pass");
        cmdBuf->ImageBarrier(_inputHDR, TextureLayout::Storage);
        cmdBuf->BindComputePipeline(_computePipeline.ComputePipeline);
        cmdBuf->PushConstantsCompute(&_settings, sizeof(_settings), 0);
        cmdBuf->Dispatch(width / 8, height / 8, 1);
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
        ImGui::SliderFloat("Hue Shift", &_settings.HueShift, -180.0f, 180.0f, "%.1f");
        ImGui::SliderFloat("Saturation", &_settings.Saturation, -100.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Temperature", &_settings.Temperature, -1.0f, 1.0f, "%.1f");
        ImGui::SliderFloat("Tint", &_settings.Tint, -1.0f, 1.0f, "%.1f");
        if (ImGui::TreeNodeEx("Split Toning", ImGuiTreeNodeFlags_Framed)) {
            ImGui::ColorPicker3("Shadows", glm::value_ptr(_settings.Shadows), ImGuiColorEditFlags_PickerHueBar);
            ImGui::ColorPicker3("Highlights", glm::value_ptr(_settings.Highlights), ImGuiColorEditFlags_PickerHueBar);
            ImGui::SliderFloat("Balance", &_settings.Balance, -100.0f, 100.0f, "%.1f");
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Color Filter", ImGuiTreeNodeFlags_Framed)) {
            ImGui::ColorPicker3("Color Filter", glm::value_ptr(_settings.ColorFilter), ImGuiColorEditFlags_PickerHueBar);
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
}

void ColorCorrection::Reconstruct()
{
    _computePipeline.CheckForRebuild(_renderContext);
}
