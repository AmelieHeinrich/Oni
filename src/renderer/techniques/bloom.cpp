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

    _downsamplePipeline.AddShaderWatch("shaders/Bloom/Compute/DownsampleCompute.hlsl", "Main", ShaderType::Compute);
    _downsamplePipeline.Build(context);

    _upsamplePipeline.AddShaderWatch("shaders/Bloom/Compute/UpsampleCompute.hlsl", "Main", ShaderType::Compute);
    _upsamplePipeline.Build(context);

    _compositePipeline.AddShaderWatch("shaders/Bloom/Compute/CompositeCompute.hlsl", "Main", ShaderType::Compute);
    _compositePipeline.Build(context);

    // Create za sampler
    _linearBorder = context->CreateSampler(SamplerAddress::Border, SamplerFilter::Linear, false, 0);
    _pointClamp = context->CreateSampler(SamplerAddress::Clamp, SamplerFilter::Nearest, false, 0);
    _linearClamp = context->CreateSampler(SamplerAddress::Clamp, SamplerFilter::Linear, false, 0);

    // Create za buffers!
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        UpsampleParameters[i] = context->CreateBuffer(256, 0, BufferType::Constant, false, "Upsample CBV (" + std::to_string(i) + ")");
        UpsampleParameters[i]->BuildConstantBuffer();
        
        CompositeParameters[i] = context->CreateBuffer(256, 0, BufferType::Constant, false, "Bloom Composite CBV (" + std::to_string(i) + ")");
        CompositeParameters[i]->BuildConstantBuffer();
    }

    // Create za mipchain!
    {
        glm::vec2 mipSize((float)width, (float)height);
        glm::ivec2 mipIntSize(width, height);

        BloomMip firstMip;
        firstMip.Size = mipSize;
        firstMip.IntSize = mipIntSize;
        firstMip.RenderTarget = inputHDR;
        for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
            firstMip.DownsampleParameters[i] = context->CreateBuffer(256, 0, BufferType::Constant, false, "Downsample CBV Mip 0 (" + std::to_string(i) + ")");
            firstMip.DownsampleParameters[i]->BuildConstantBuffer();
        }

        _mipChain.push_back(firstMip);

        for (int i = 1; i < MIP_COUNT; i++) {
            BloomMip mip;
            
            mipSize *= 0.5f;
            mipIntSize /= 2;
            mip.Size = mipSize;
            mip.IntSize = mipIntSize;

            for (int j = 0; j < FRAMES_IN_FLIGHT; j++) {
                mip.DownsampleParameters[j] = context->CreateBuffer(256, 0, BufferType::Constant, false, "Downsample CBV Mip " + std::to_string(i) + " (" + std::to_string(j) + ")");
                mip.DownsampleParameters[j]->BuildConstantBuffer();
            }

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

        struct Data {
            glm::vec2 SourceResolution;
            glm::vec2 Pad;
        };
        Data data;
        data.SourceResolution = mip.IntSize;

        void *pData;
        mip.DownsampleParameters[frameIndex]->Map(0, 0, &pData);
        memcpy(pData, &data, sizeof(Data));
        mip.DownsampleParameters[frameIndex]->Unmap(0, 0);
        
        cmdBuf->ImageBarrier(_mipChain[i].RenderTarget, TextureLayout::ShaderResource);
        cmdBuf->ImageBarrier(_mipChain[i + 1].RenderTarget, TextureLayout::Storage);
        cmdBuf->BindComputePipeline(_downsamplePipeline.ComputePipeline);
        cmdBuf->BindComputeShaderResource(_mipChain[i].RenderTarget, 0, 0);
        cmdBuf->BindComputeSampler(_linearClamp, 1);
        cmdBuf->BindComputeStorageTexture(_mipChain[i + 1].RenderTarget, 2, 0);
        //cmdBuf->BindComputeConstantBuffer(mip.DownsampleParameters[frameIndex], 3);
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

    struct Data {
        float FilterRadius;
        glm::vec3 Pad;
    };
    Data data;
    data.FilterRadius = _filterRadius;

    void *pData;
    UpsampleParameters[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &data, sizeof(Data));
    UpsampleParameters[frameIndex]->Unmap(0, 0);

    cmdBuf->BeginEvent("Bloom Upsample");
    cmdBuf->ImageBarrier(_output, TextureLayout::ShaderResource);
    for (int i = MIP_COUNT - 1; i > 1; i--) {
        const auto& mip = _mipChain[i - 1];
        
        cmdBuf->ImageBarrier(_mipChain[i].RenderTarget, TextureLayout::ShaderResource);
        cmdBuf->ImageBarrier(_mipChain[i - 1].RenderTarget, TextureLayout::Storage);
        cmdBuf->BindComputePipeline(_upsamplePipeline.ComputePipeline);

        cmdBuf->BindComputeShaderResource(_mipChain[i].RenderTarget, 0, 0);
        cmdBuf->BindComputeSampler(_linearClamp, 1);
        cmdBuf->BindComputeStorageTexture(_mipChain[i - 1].RenderTarget, 2, 0);
        cmdBuf->BindComputeConstantBuffer(UpsampleParameters[frameIndex], 3);

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

    struct Data {
        float BloomStrength;
        glm::vec3 Pad;
    };
    Data data;
    data.BloomStrength = _bloomStrenght;

    void *pData;
    CompositeParameters[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &data, sizeof(Data));
    CompositeParameters[frameIndex]->Unmap(0, 0);

    BloomMip firstMip = _mipChain[1];

    cmdBuf->BeginEvent("Bloom Composite");
    cmdBuf->ImageBarrier(firstMip.RenderTarget, TextureLayout::ShaderResource);
    cmdBuf->ImageBarrier(_output, TextureLayout::Storage);
    cmdBuf->BindComputePipeline(_compositePipeline.ComputePipeline);
    cmdBuf->BindComputeShaderResource(firstMip.RenderTarget, 0, 0);
    cmdBuf->BindComputeSampler(_linearClamp, 1);
    cmdBuf->BindComputeStorageTexture(_output, 2, 0);
    cmdBuf->BindComputeConstantBuffer(CompositeParameters[frameIndex], 3);
    cmdBuf->Dispatch(std::max(width / 8u, 1u), std::max(height / 8u, 1u), 1);
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
    firstMip.RenderTarget = inputHDR;
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        firstMip.DownsampleParameters[i] = _context->CreateBuffer(256, 0, BufferType::Constant, false, "Downsample CBV Mip 0 (" + std::to_string(i) + ")");
        firstMip.DownsampleParameters[i]->BuildConstantBuffer();
    }

    _mipChain.push_back(firstMip);

    for (int i = 1; i < MIP_COUNT; i++) {
        BloomMip mip;
        
        mipSize *= 0.5f;
        mipIntSize /= 2;
        mip.Size = mipSize;
        mip.IntSize = mipIntSize;

        for (int j = 0; j < FRAMES_IN_FLIGHT; j++) {
            mip.DownsampleParameters[j] = _context->CreateBuffer(256, 0, BufferType::Constant, false, "Downsample CBV Mip " + std::to_string(i) + " (" + std::to_string(j) + ")");
            mip.DownsampleParameters[j]->BuildConstantBuffer();
        }

        mip.RenderTarget = _context->CreateTexture((uint32_t)mipSize.x, (uint32_t)mipSize.y, TextureFormat::RGB11Float, TextureUsage::ShaderResource, false, "Bloom Mip " + std::to_string(i));
        mip.RenderTarget->BuildShaderResource();
        mip.RenderTarget->BuildStorage();
        _mipChain.push_back(mip);
    }
}

void Bloom::OnUI()
{
    if (ImGui::TreeNodeEx("Bloom", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Enable", &_enable);
        ImGui::SliderFloat("Filter Radius", &_filterRadius, 0.0f, 0.01f, "%.3f");
        ImGui::SliderFloat("Bloom Strength", &_bloomStrenght, 0.0f, 0.7f, "%.2f");
        ImGui::TreePop();
    }
}

void Bloom::Reconstruct()
{
    _downsamplePipeline.CheckForRebuild(_context);
    _upsamplePipeline.CheckForRebuild(_context);
    _compositePipeline.CheckForRebuild(_context);
}
