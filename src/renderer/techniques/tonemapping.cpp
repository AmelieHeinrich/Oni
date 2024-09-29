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

    _outputLDR = _renderContext->CreateTexture(width, height, TextureFormat::RGBA8, TextureUsage::RenderTarget, false, "[TONEMAPPING] Tonemapping LDR Output");
    _outputLDR->BuildShaderResource();
    _outputLDR->BuildStorage();
    _outputLDR->BuildRenderTarget();

    _computePipeline.SignatureInfo.Entries = {
        RootSignatureEntry::PushConstants
    };
    _computePipeline.SignatureInfo.PushConstantSize = sizeof(glm::ivec4);

    _computePipeline.ReflectRootSignature(false);
    _computePipeline.AddShaderWatch("shaders/Tonemapping/TonemappingCompute.hlsl", "Main", ShaderType::Compute);
    _computePipeline.Build(context);
}

Tonemapping::~Tonemapping()
{
}

void Tonemapping::Render(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr cmdBuf = _renderContext->GetCurrentCommandBuffer();

    struct PushConstants {
        uint32_t Mode;
        uint32_t HDRTexture;
        uint32_t LDRTexture;
        uint32_t _Pad0;
    };
    PushConstants constants = {
        _tonemapper,
        _inputHDR->SRV(),
        _outputLDR->UAV(),
        0
    };

    OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
    OPTICK_GPU_EVENT("Tonemapping PostFX");

    cmdBuf->BeginEvent("Tonemapping Pass");
    cmdBuf->ImageBarrierBatch({
        { _inputHDR, TextureLayout::ShaderResource },
        { _outputLDR, TextureLayout::Storage }
    });
    cmdBuf->BindComputePipeline(_computePipeline.ComputePipeline);
    cmdBuf->PushConstantsCompute(&constants, sizeof(PushConstants), 0);
    cmdBuf->Dispatch(std::ceil(width / 8), std::ceil(height / 8), 1);
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
    _computePipeline.CheckForRebuild(_renderContext, "Tonemapping");
}
