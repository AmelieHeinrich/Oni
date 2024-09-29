/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-11 12:24:54
 */

#include "auto_exposure.hpp"

#include <ImGui/imgui.h>
#include <optick.h>

AutoExposure::AutoExposure(RenderContext::Ptr context, Texture::Ptr inputHDR)
    : _inputHDR(inputHDR), _renderContext(context), _computePipeline(PipelineType::Compute), _averagePipeline(PipelineType::Compute)
{
    {
        _computePipeline.SignatureInfo = {
            { RootSignatureEntry::PushConstants },
            sizeof(float) * 6
        };
        _computePipeline.ReflectRootSignature(false);
        _computePipeline.AddShaderWatch("shaders/AutoExposure/LuminanceHistogramCompute.hlsl", "Main", ShaderType::Compute);
        _computePipeline.Build(context);
    }
    {
        _averagePipeline.SignatureInfo = {
            { RootSignatureEntry::PushConstants },
            sizeof(float) * 7
        };
        _averagePipeline.ReflectRootSignature(false);
        _averagePipeline.AddShaderWatch("shaders/AutoExposure/HistogramAverageCompute.hlsl", "Main", ShaderType::Compute);
        _averagePipeline.Build(context);
    }

    _luminanceHistogram = _renderContext->CreateBuffer(256 * 4, 0, BufferType::Storage, false, "[AUTOEXPOSURE] Luminance Histogram");
    _luminanceHistogram->BuildStorage();

    _luminanceTexture = _renderContext->CreateTexture(1, 1, TextureFormat::R32Float, TextureUsage::Storage, false, "[AUTOEXPOSURE] Luminance Texture");
    _luminanceTexture->BuildShaderResource();
    _luminanceTexture->BuildStorage();
}

AutoExposure::~AutoExposure()
{

}

void AutoExposure::Render(Scene& scene, uint32_t width, uint32_t height, float dt)
{
    if (_enable) {
        CommandBuffer::Ptr cmdBuf = _renderContext->GetCurrentCommandBuffer();

        // Compute the histogram
        {
            OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
            OPTICK_GPU_EVENT("Compute Auto Exposure Histogram");

            struct {
                uint32_t inputHDR;
                uint32_t luminanceHistogram;
                uint32_t width;
                uint32_t height;
                float minLogLuminance;
                float oneOverLogLuminanceRange;
            } data;

            data.inputHDR = _inputHDR->SRV();
            data.luminanceHistogram = _luminanceHistogram->UAV();
            data.width = width;
            data.height = height;
            data.minLogLuminance = _minLogLuminance;
            data.oneOverLogLuminanceRange = 1.0f / _luminanceRange;

            cmdBuf->BeginEvent("AE Histogram Compute Pass");
            cmdBuf->BindComputePipeline(_computePipeline.ComputePipeline);
            cmdBuf->PushConstantsCompute(&data, sizeof(data), 0);
            cmdBuf->Dispatch(round(width / 16), round(height / 16), 1);
            cmdBuf->EndEvent();
        }

        // Average the histogram
        {
            OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
            OPTICK_GPU_EVENT("Average Auto Exposure Histogram");

            struct {
                uint32_t LuminanceHistogram;
                uint32_t LuminanceOutput;
                uint32_t pixelCount;
                float minLogLuminance;
                float logLuminanceRange;
                float timeDelta;
                float tau;
            } data;
            data.LuminanceHistogram = _luminanceHistogram->UAV();
            data.LuminanceOutput = _luminanceTexture->UAV();
            data.pixelCount = width * height;
            data.minLogLuminance = _minLogLuminance;
            data.logLuminanceRange = _luminanceRange;
            data.timeDelta = dt;
            data.tau = _tau;

            cmdBuf->BeginEvent("AE Histogram Average Compute Pass");
            cmdBuf->ImageBarrier(_luminanceTexture, TextureLayout::Storage);
            cmdBuf->BindComputePipeline(_averagePipeline.ComputePipeline);
            cmdBuf->PushConstantsCompute(&data, sizeof(data), 0);
            cmdBuf->Dispatch(round(width / 16), round(height / 16), 1);
            cmdBuf->ImageBarrier(_luminanceTexture, TextureLayout::ShaderResource);
            cmdBuf->EndEvent();
        }
    }
}

void AutoExposure::Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR)
{
    _inputHDR = inputHDR;
}

void AutoExposure::OnUI()
{
    if (ImGui::TreeNodeEx("Auto Exposure", ImGuiTreeNodeFlags_Framed))
    {
        ImGui::Checkbox("Enable", &_enable);
        ImGui::Separator();

        ImGui::TreePop();
    }
}

void AutoExposure::Reconstruct()
{
    _computePipeline.CheckForRebuild(_renderContext, "AE Histogram Compute");
    _averagePipeline.CheckForRebuild(_renderContext, "AE Histogram Average");
}
