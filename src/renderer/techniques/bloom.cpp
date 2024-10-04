//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 12:35:27
//

#include "bloom.hpp"

#include <cmath>

Bloom::Bloom(RenderContext::Ptr context, Texture::Ptr inputHDR)
    : _context(context), _downsamplePipeline(PipelineType::Compute), _upsamplePipeline(PipelineType::Compute), _compositePipeline(PipelineType::Compute), _output(inputHDR)
{
    uint32_t width, height;
    context->GetWindow()->GetSize(width, height);

    _downsamplePipeline.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(glm::ivec3)
    };
    _downsamplePipeline.ReflectRootSignature(false);
    _downsamplePipeline.AddShaderWatch("shaders/Bloom/DownsampleCompute.hlsl", "Main", ShaderType::Compute);
    _downsamplePipeline.Build(context);

    _upsamplePipeline.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(glm::vec4)
    };
    _upsamplePipeline.ReflectRootSignature(false);
    _upsamplePipeline.AddShaderWatch("shaders/Bloom/UpsampleCompute.hlsl", "Main", ShaderType::Compute);
    _upsamplePipeline.Build(context);

    _compositePipeline.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(glm::vec4)
    };
    _compositePipeline.ReflectRootSignature(false);
    _compositePipeline.AddShaderWatch("shaders/Bloom/CompositeCompute.hlsl", "Main", ShaderType::Compute);
    _compositePipeline.Build(context);

    // Create za sampler
    _linearBorder = context->CreateSampler(SamplerAddress::Border, SamplerFilter::Linear, false, 0);
    _pointClamp = context->CreateSampler(SamplerAddress::Clamp, SamplerFilter::Nearest, false, 0);
    _linearClamp = context->CreateSampler(SamplerAddress::Clamp, SamplerFilter::Linear, false, 0);

    _bloomFramebuffer = context->CreateTexture(width, height, TextureFormat::RGBA16Float, TextureUsage::Storage, true, "[BLOOM] Bloom Framebuffer");
    _bloomFramebuffer->BuildStorage();
    _bloomFramebuffer->BuildShaderResource();
}

void Bloom::Downsample(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr cmdBuf = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
    OPTICK_GPU_EVENT("Bloom Downsample");

    cmdBuf->BeginEvent("Copy Emission to First Mip");
    cmdBuf->ImageBarrierBatch({
        { _bloomFramebuffer, TextureLayout::CopyDest },
        { _emissionBuffer, TextureLayout::CopySource }
    });
    cmdBuf->CopyTextureToTexture(_bloomFramebuffer, _emissionBuffer);
    cmdBuf->ImageBarrierBatch({
        { _bloomFramebuffer, TextureLayout::Storage },
        { _emissionBuffer, TextureLayout::RenderTarget }
    });
    cmdBuf->EndEvent();

    cmdBuf->BeginEvent("Bloom Downsample");
    for (int i = 0; i < MIP_CAP; i++) {
        int width = (int)(_bloomFramebuffer->GetWidth() * pow(0.5f, i));
        int height = (int)(_bloomFramebuffer->GetHeight() * pow(0.5f, i));

        cmdBuf->ImageBarrierBatch({
            { _bloomFramebuffer, TextureLayout::ShaderResource, i },
            { _bloomFramebuffer, TextureLayout::Storage, i + 1 }
        });
        cmdBuf->BindComputePipeline(_downsamplePipeline.ComputePipeline);
        cmdBuf->PushConstantsCompute(glm::value_ptr(glm::ivec3(_bloomFramebuffer->SRV(i), _linearClamp->BindlesssSampler(), _bloomFramebuffer->UAV(i + 1))), sizeof(glm::ivec3), 0);
        cmdBuf->Dispatch(std::max(uint32_t(width) / 8u, 1u), std::max(uint32_t(height) / 8u, 1u), 1);
    }
    cmdBuf->EndEvent();
}

void Bloom::Upsample(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr cmdBuf = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
    OPTICK_GPU_EVENT("Bloom Upsample");

    cmdBuf->BeginEvent("Bloom Upsample");
    for (int i = MIP_CAP - 1; i > 0; i--) {
        int width = (int)(_bloomFramebuffer->GetWidth() * pow(0.5f, i - 1));
        int height = (int)(_bloomFramebuffer->GetHeight() * pow(0.5f, i - 1));
        
        struct Data {
            float FilterRadius;
            uint32_t MipN;
            uint32_t LinearSampler;
            uint32_t MipNMinusOne;
        };
        Data data;
        data.FilterRadius = _filterRadius;
        data.MipN = _bloomFramebuffer->SRV(i);
        data.LinearSampler = _linearClamp->BindlesssSampler();
        data.MipNMinusOne = _bloomFramebuffer->UAV(i - 1);

        cmdBuf->ImageBarrierBatch({
            { _bloomFramebuffer, TextureLayout::ShaderResource, i },
            { _bloomFramebuffer, TextureLayout::Storage, i -1 }
        });
        cmdBuf->BindComputePipeline(_upsamplePipeline.ComputePipeline);
        cmdBuf->PushConstantsCompute(&data, sizeof(data), 0);
        cmdBuf->Dispatch(std::max(uint32_t(width) / 4u, 1u),
                         std::max(uint32_t(height) / 4u, 1u),
                         1);
        cmdBuf->ImageBarrierBatch({
            { _bloomFramebuffer, TextureLayout::ShaderResource, i },
            { _bloomFramebuffer, TextureLayout::ShaderResource, i -1 }
        });
    }
    cmdBuf->EndEvent();
}

void Bloom::Composite(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr cmdBuf = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
    OPTICK_GPU_EVENT("Bloom Composite");

    struct Data {
        uint32_t Input;
        uint32_t InputSampler;
        uint32_t OutputHDR;
        float BloomStrength;
    };
    Data data;
    data.Input = _bloomFramebuffer->SRV(0);
    data.InputSampler = _linearClamp->BindlesssSampler();
    data.OutputHDR = _output->UAV();
    data.BloomStrength = _bloomStrenght;

    cmdBuf->BeginEvent("Bloom Composite");
    cmdBuf->ImageBarrierBatch({
        { _bloomFramebuffer, TextureLayout::ShaderResource },
        { _output, TextureLayout::Storage }
    });
    cmdBuf->BindComputePipeline(_compositePipeline.ComputePipeline);
    cmdBuf->PushConstantsCompute(&data, sizeof(data), 0);
    cmdBuf->Dispatch(std::max((uint32_t)std::ceil(width / 8u), 1u), std::max((uint32_t)std::ceil(height / 8u), 1u), 1);
    cmdBuf->EndEvent();
}

void Bloom::Render(Scene& scene, uint32_t width, uint32_t height)
{
    if (_enable) {
        CommandBuffer::Ptr cmdBuf = _context->GetCurrentCommandBuffer();

        cmdBuf->BeginEvent("Bloom");
        Downsample(scene, width, height);
        Upsample(scene, width, height);
        Composite(scene, width, height);
        cmdBuf->EndEvent();
    }
}

void Bloom::Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR)
{

}

void Bloom::OnUI()
{
    if (ImGui::TreeNodeEx("Bloom", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Enable", &_enable);
        ImGui::SliderFloat("Strength", &_bloomStrenght, 0.0f, 4.0f, "%.1f");
        ImGui::TreePop();
    }
}

void Bloom::Reconstruct()
{
    _downsamplePipeline.CheckForRebuild(_context, "Bloom Downsample");
    _upsamplePipeline.CheckForRebuild(_context, "Bloom Upsamble");
    _compositePipeline.CheckForRebuild(_context, "Bloom Composite");
}

void Bloom::ConnectEmissiveBuffer(Texture::Ptr texture)
{
    _emissionBuffer = texture;
}
