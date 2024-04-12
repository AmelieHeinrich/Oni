/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 17:24:42
 */

#include "color_correction.hpp"

#include <ImGui/imgui.h>

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

        void *pData;
        _correctionParameters->Map(0, 0, &pData);
        memcpy(pData, &_settings, sizeof(ColorCorrectionSettings));
        _correctionParameters->Unmap(0, 0);

        cmdBuf->Begin();
        cmdBuf->BeginEvent("Color Correction Pass");
        cmdBuf->ImageBarrier(_inputHDR, TextureLayout::Storage);
        cmdBuf->BindComputePipeline(_computePipeline);
        cmdBuf->BindComputeShaderResource(_inputHDR, 0, 0);
        cmdBuf->BindComputeConstantBuffer(_correctionParameters, 1);
        cmdBuf->Dispatch(width / 30, height / 30, 1);
        cmdBuf->ImageBarrier(_inputHDR, TextureLayout::RenderTarget);
        cmdBuf->EndEvent();
        cmdBuf->End();
        _renderContext->ExecuteCommandBuffers({ cmdBuf }, CommandQueueType::Graphics);
    }
}

void ColorCorrection::Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR)
{
    _inputHDR = inputHDR;
}

void ColorCorrection::OnUI()
{
    if (ImGui::TreeNodeEx("Color Correction", ImGuiTreeNodeFlags_Framed))
    {
        ImGui::Checkbox("Enable", &_enable);
        ImGui::Separator();

        ImGui::SliderFloat("Exposure", &_settings.Exposure, 0.1f, 5.0f, "%.1f");
        ImGui::SliderFloat("Temperature", &_settings.Temperature, 0.0f, 1.0f, "%.1f");
        ImGui::SliderFloat("Tint", &_settings.Tint, 0.0f, 1.0f, "%.1f");
        ImGui::SliderFloat3("Contrast", glm::value_ptr(_settings.Contrast), 0.0f, 5.0f, "%.1f");
        ImGui::SliderFloat3("Linear Mid Point", glm::value_ptr(_settings.LinearMidPoint), 0.0f, 5.0f, "%.1f");
        ImGui::SliderFloat3("Brightness", glm::value_ptr(_settings.Brightness), 0.0f, 5.0f, "%.1f");
        ImGui::ColorPicker3("Color Filter", glm::value_ptr(_settings.ColorFilter));
        ImGui::SliderFloat("Color Filter Intensity", &_settings.ColorFilterIntensity, 0.0f, 5.0f, "%.1f");
        ImGui::SliderFloat3("Saturation", glm::value_ptr(_settings.Saturation), 0.0f, 5.0f, "%.1f");

        ImGui::TreePop();
    }
}
