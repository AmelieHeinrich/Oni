/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 17:24:42
 */

#include "tonemapping.hpp"

#include <ImGui/imgui.h>
#include <optick.h>

Tonemapping::Tonemapping(RenderContext::Ptr context, Texture::Ptr inputHDR)
    : _renderContext(context), _inputHDR(inputHDR), _computePipeline(PipelineType::Compute)
{
    uint32_t width, height;
    context->GetWindow()->GetSize(width, height);

    _outputLDR = _renderContext->CreateTexture(width, height, TextureFormat::RGBA8, TextureUsage::RenderTarget, false, "Tonemapping LDR Output");
    _outputLDR->BuildShaderResource();
    _outputLDR->BuildStorage();
    _outputLDR->BuildRenderTarget();

    _computePipeline.AddShaderWatch("shaders/Tonemapping/TonemappingCompute.hlsl", "Main", ShaderType::Compute);
    _computePipeline.Build(context);

    _tonemapperSettings = _renderContext->CreateBuffer(256, 0, BufferType::Constant, false, "Tonemapper Settings CBV");
    _tonemapperSettings->BuildConstantBuffer();
}

Tonemapping::~Tonemapping()
{
}

void Tonemapping::Render(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr cmdBuf = _renderContext->GetCurrentCommandBuffer();

    OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
    OPTICK_GPU_EVENT("Tonemapping PostFX");

    void *pData;
    _tonemapperSettings->Map(0, 0, &pData);
    memcpy(pData, glm::value_ptr(glm::ivec4(_tonemapper, 0, 0, 0)), sizeof(glm::ivec4));
    _tonemapperSettings->Unmap(0, 0);

    cmdBuf->BeginEvent("Tonemapping Pass");
    cmdBuf->ImageBarrier(_inputHDR, TextureLayout::ShaderResource);
    cmdBuf->ImageBarrier(_outputLDR, TextureLayout::Storage);
    cmdBuf->BindComputePipeline(_computePipeline.ComputePipeline);
    cmdBuf->BindComputeShaderResource(_inputHDR, 0, 0);
    cmdBuf->BindComputeStorageTexture(_outputLDR, 1, 0);
    cmdBuf->BindComputeConstantBuffer(_tonemapperSettings, 2);
    cmdBuf->Dispatch(width / 32, height / 32, 1);    
    cmdBuf->EndEvent();
}

void Tonemapping::Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR)
{
    _inputHDR = inputHDR;

    _outputLDR.reset();
    _outputLDR = _renderContext->CreateTexture(width, height, TextureFormat::RGBA8, TextureUsage::RenderTarget, false,"Tonemapping LDR Output");
    _outputLDR->BuildStorage();
    _outputLDR->BuildShaderResource();
    _outputLDR->BuildRenderTarget();
}

void Tonemapping::OnUI()
{
    if (ImGui::TreeNodeEx("Tonemapping", ImGuiTreeNodeFlags_Framed))
    {
        static const char* Tonemappers[] = { "ACES", "Filmic", "Rom Bin Da House" };
        ImGui::Combo("Tonemapper", (int*)&_tonemapper, Tonemappers, 3);

        ImGui::TreePop();
    }
}

void Tonemapping::Reconstruct()
{
    _computePipeline.CheckForRebuild(_renderContext);
}
