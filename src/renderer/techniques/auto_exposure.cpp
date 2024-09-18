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
        _computePipeline.AddShaderWatch("shaders/AutoExposure/LuminanceHistogram.hlsl", "Main", ShaderType::Compute);
        _computePipeline.Build(context);
    }
    {
        _averagePipeline.AddShaderWatch("shaders/AutoExposure/HistogramAverage.hlsl", "Main", ShaderType::Compute);
        _averagePipeline.Build(context);
    }

    _luminanceHistogram = _renderContext->CreateBuffer(256 * 4, 0, BufferType::Storage, false, "Luminance Histogram");
    _luminanceHistogram->BuildStorage();

    _luminanceHistogramParameters = _renderContext->CreateBuffer(256, 0, BufferType::Constant, false, "Luminance Histogram CBV");
    _luminanceHistogramParameters->BuildConstantBuffer();

    _averageParameters = _renderContext->CreateBuffer(256, 0, BufferType::Constant, false, "Average Histogram CBV");
    _averageParameters->BuildConstantBuffer();

    _luminanceTexture = _renderContext->CreateTexture(1, 1, TextureFormat::R32Float, TextureUsage::Storage, false, "Luminance Texture");
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
                uint32_t width;
                uint32_t height;
                float minLogLuminance;
                float oneOverLogLuminanceRange;
            } data;

            data.width = width;
            data.height = height;
            data.minLogLuminance = _minLogLuminance;
            data.oneOverLogLuminanceRange = 1.0f / _luminanceRange;

            void *pData;
            _luminanceHistogramParameters->Map(0, 0, &pData);
            memcpy(pData, &data, sizeof(data));
            _luminanceHistogramParameters->Unmap(0, 0);

            cmdBuf->BeginEvent("AE Histogram Compute Pass");
            cmdBuf->BindComputePipeline(_computePipeline.ComputePipeline);
            cmdBuf->BindComputeShaderResource(_inputHDR, 0, 0);
            cmdBuf->BindComputeStorageBuffer(_luminanceHistogram, 1);
            cmdBuf->BindComputeConstantBuffer(_luminanceHistogramParameters, 2);
            cmdBuf->Dispatch(round(width / 16), round(height / 16), 1);
            cmdBuf->EndEvent();
        }

        // Average the histogram
        {
            OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
            OPTICK_GPU_EVENT("Average Auto Exposure Histogram");

            struct {
                uint32_t pixelCount;
                float minLogLuminance;
                float logLuminanceRange;
                float timeDelta;
                float tau;
            } data;

            data.pixelCount = width * height;
            data.minLogLuminance = _minLogLuminance;
            data.logLuminanceRange = _luminanceRange;
            data.timeDelta = dt;
            data.tau = _tau;

            void *pData;
            _averageParameters->Map(0, 0, &pData);
            memcpy(pData, &data, sizeof(data));
            _averageParameters->Unmap(0, 0);

            cmdBuf->BeginEvent("AE Histogram Average Compute Pass");
            cmdBuf->ImageBarrier(_luminanceTexture, TextureLayout::Storage);
            cmdBuf->BindComputePipeline(_averagePipeline.ComputePipeline);
            cmdBuf->BindComputeStorageBuffer(_luminanceHistogram, 0);
            cmdBuf->BindComputeStorageTexture(_luminanceTexture, 1, 0);
            cmdBuf->BindComputeConstantBuffer(_averageParameters, 2);
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
    _computePipeline.CheckForRebuild(_renderContext);
    _averagePipeline.CheckForRebuild(_renderContext);
}
