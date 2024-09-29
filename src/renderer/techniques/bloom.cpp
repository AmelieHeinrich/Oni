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

    // Create za mipchain!
    {
        glm::vec2 mipSize((float)width, (float)height);
        glm::ivec2 mipIntSize(width, height);

        BloomMip firstMip;
        firstMip.Size = mipSize;
        firstMip.IntSize = mipIntSize;
        firstMip.RenderTarget = nullptr;

        _mipChain.push_back(firstMip);

        for (int i = 1; i < MIP_COUNT; i++) {
            BloomMip mip;
            
            mipSize *= 0.5f;
            mipIntSize /= 2;
            mip.Size = mipSize;
            mip.IntSize = mipIntSize;

            mip.RenderTarget = context->CreateTexture((uint32_t)mipSize.x, (uint32_t)mipSize.y, TextureFormat::RGB11Float, TextureUsage::ShaderResource, false, "Bloom Mip " + std::to_string(i));
            mip.RenderTarget->BuildShaderResource();
            mip.RenderTarget->BuildStorage();
            _mipChain.push_back(mip);
        }
    }
}

void Bloom::Downsample(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr cmdBuf = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
    OPTICK_GPU_EVENT("Bloom Downsample");

    cmdBuf->BeginEvent("Bloom Downsample");
    for (int i = 0; i < MIP_COUNT - 1; i++) {
        const auto& mip = _mipChain[i];
        
        cmdBuf->ImageBarrierBatch({
            { _mipChain[i].RenderTarget, TextureLayout::ShaderResource },
            { _mipChain[i + 1].RenderTarget, TextureLayout::Storage }
        });
        cmdBuf->BindComputePipeline(_downsamplePipeline.ComputePipeline);
        cmdBuf->PushConstantsCompute(glm::value_ptr(glm::ivec3(_mipChain[i].RenderTarget->SRV(), _linearClamp->BindlesssSampler(), _mipChain[i + 1].RenderTarget->UAV())), sizeof(glm::ivec3), 0);
        cmdBuf->Dispatch(std::max(uint32_t(mip.Size.x) / 8u, 1u), std::max(uint32_t(mip.Size.y) / 8u, 1u), 1);
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
    cmdBuf->ImageBarrier(_output, TextureLayout::ShaderResource);
    for (int i = MIP_COUNT - 1; i > 1; i--) {
        const auto& mip = _mipChain[i - 1];
        
        struct Data {
            float FilterRadius;
            uint32_t MipN;
	        uint32_t LinearSampler;
	        uint32_t MipNMinusOne;
        };
        Data data;
        data.FilterRadius = _filterRadius;
        data.MipN = _mipChain[i].RenderTarget->SRV();
        data.LinearSampler = _linearClamp->BindlesssSampler();
        data.MipNMinusOne = mip.RenderTarget->UAV();

        cmdBuf->ImageBarrierBatch({
            { _mipChain[i].RenderTarget, TextureLayout::ShaderResource },
            { _mipChain[i - 1].RenderTarget, TextureLayout::Storage }
        });
        cmdBuf->BindComputePipeline(_upsamplePipeline.ComputePipeline);
        cmdBuf->PushConstantsCompute(&data, sizeof(data), 0);
        cmdBuf->Dispatch(std::max(uint32_t(mip.Size.x) / 8u, 1u),
                         std::max(uint32_t(mip.Size.y) / 8u, 1u),
                         1);
    }
    cmdBuf->EndEvent();
}

void Bloom::Composite(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr cmdBuf = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
    OPTICK_GPU_EVENT("Bloom Composite");

    BloomMip firstMip = _mipChain[1];

    struct Data {
        uint32_t Input;
        uint32_t InputSampler;
        uint32_t OutputHDR;
        float BloomStrength;
    };
    Data data;
    data.Input = firstMip.RenderTarget->SRV();
    data.InputSampler = _linearClamp->BindlesssSampler();
    data.OutputHDR = _output->UAV();
    data.BloomStrength = _bloomStrenght;

    cmdBuf->BeginEvent("Bloom Composite");
    cmdBuf->ImageBarrierBatch({
        { firstMip.RenderTarget, TextureLayout::ShaderResource },
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
        Downsample(scene, width, height);
        Upsample(scene, width, height);
        Composite(scene, width, height);
    }
}

void Bloom::Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR)
{
    _mipChain.clear();

    glm::vec2 mipSize((float)width, (float)height);
    glm::ivec2 mipIntSize(width, height);

    BloomMip firstMip;
    firstMip.Size = mipSize;
    firstMip.IntSize = mipIntSize;
    firstMip.RenderTarget = _emissionBuffer;

    _mipChain.push_back(firstMip);

    for (int i = 1; i < MIP_COUNT; i++) {
        BloomMip mip;
        
        mipSize *= 0.5f;
        mipIntSize /= 2;
        mip.Size = mipSize;
        mip.IntSize = mipIntSize;

        mip.RenderTarget = _context->CreateTexture((uint32_t)mipSize.x, (uint32_t)mipSize.y, TextureFormat::RGB11Float, TextureUsage::ShaderResource, false, "[BLOOM] Bloom Mip " + std::to_string(i));
        mip.RenderTarget->BuildShaderResource();
        mip.RenderTarget->BuildStorage();
        _mipChain.push_back(mip);
    }
}

void Bloom::OnUI()
{
    if (ImGui::TreeNodeEx("Bloom", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Enable", &_enable);
        //ImGui::SliderFloat("Filter Radius", &_filterRadius, 0.0f, 0.01f, "%.3f");
        //ImGui::SliderFloat("Bloom Strength", &_bloomStrenght, 0.0f, 0.7f, "%.2f");
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
    _mipChain[0].RenderTarget = _emissionBuffer;
}
