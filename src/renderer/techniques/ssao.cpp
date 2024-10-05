//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-05 11:36:48
//

#include "ssao.hpp"
#include "core/util.hpp"

SSAO::SSAO(RenderContext::Ptr renderContext)
    : _context(renderContext), _ssaoPipeline(PipelineType::Compute), _ssaoBlur(PipelineType::Compute)
{
    uint32_t width, height;
    renderContext->GetWindow()->GetSize(width, height);

    // Create pipelines
    _ssaoPipeline.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(glm::vec4) * 3
    };
    _ssaoPipeline.ReflectRootSignature(false);
    _ssaoPipeline.AddShaderWatch("shaders/SSAO/SSAOCompute.hlsl", "Main", ShaderType::Compute);
    _ssaoPipeline.Build(_context);

    _ssaoBlur.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(glm::uvec4)
    };
    _ssaoBlur.ReflectRootSignature(false);
    _ssaoBlur.AddShaderWatch("shaders/SSAO/SSAOBoxBlurCompute.hlsl", "Main", ShaderType::Compute);
    _ssaoBlur.Build(_context);

    // Generate kernels and noise
    std::vector<glm::vec3> kernels;
    for (int i = 0; i < _kernelSize; i++) {
        glm::vec3 sample(util::random_range(-1.0, 1.0), util::random_range(-1.0, 1.0), util::random_range(0.0, 1.0));
        
        float scale = (float)i / (float)_kernelSize;
        float scaleMul = util::lerp(0.1f, 1.0f, scale * scale);
        sample *= scaleMul;

        kernels.push_back(sample);
    }

    std::vector<glm::vec4> noiseData;
    for (int i = 0; i < 16; i++) {
        glm::vec3 noise(util::random_range(-1.0, 1.0), util::random_range(-1.0, 1.0), 0.0f);
        noiseData.push_back(glm::vec4(noise, 1.0f));
    }

    // Create textures and buffer
    _ssao = _context->CreateTexture(width, height, TextureFormat::R32Float, TextureUsage::Storage, false, "[SSAO] SSAO Texture");
    _ssao->BuildStorage();
    _ssao->BuildShaderResource();

    _noise = _context->CreateTexture(4, 4, TextureFormat::RGBA32Float, TextureUsage::ShaderResource, false, "[SSAO] Noise Texture");
    _noise->BuildShaderResource();

    _kernelBuffer = _context->CreateBuffer(sizeof(glm::vec3) * 64, 0, BufferType::Constant, false, "[SSAO] Kernel Buffer");
    _kernelBuffer->BuildConstantBuffer();

    void *pData;
    _kernelBuffer->Map(0, 0, &pData);
    memcpy(pData, kernels.data(), kernels.size() * sizeof(glm::vec3));
    _kernelBuffer->Unmap(0, 0);

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        _cameraBuffer[i] = _context->CreateBuffer(256, 0, BufferType::Constant, false, "[SSAO] Camera Buffer (FIF " + std::to_string(i) + ")");
        _cameraBuffer[i]->BuildConstantBuffer();
    }

    // Create sampler
    _pointSampler = _context->CreateSampler(SamplerAddress::Wrap, SamplerFilter::Nearest, false, 0);
    _pointClampSampler = _context->CreateSampler(SamplerAddress::Clamp, SamplerFilter::Nearest, false, 0);

    // Upload noise bitmap
    Bitmap noiseBitmap;
    noiseBitmap.Width = _noise->GetWidth();
    noiseBitmap.Height = _noise->GetHeight();
    noiseBitmap.Delete = false;
    noiseBitmap.Bytes = reinterpret_cast<char*>(noiseData.data());

    Uploader uploader = _context->CreateUploader();
    uploader.CopyHostToDeviceTexture(noiseBitmap, _noise);
    _context->FlushUploader(uploader);

    _kernelSize = 16;
}

void SSAO::Render(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();

    commandBuffer->BeginEvent("SSAO");
    SSAOPass(scene, width, height);
    BlurPass(scene, width, height);
    commandBuffer->EndEvent();
}

void SSAO::SSAOPass(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    // Upload camera buffer
    glm::mat4 matrices[2] = {
        scene.Camera.Projection(),
        glm::inverse(scene.Camera.Projection() * scene.Camera.View())
    };
    void *pData;
    _cameraBuffer[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, matrices, sizeof(matrices));
    _cameraBuffer[frameIndex]->Unmap(0, 0);

    // Upload push constants
    struct Data {
        uint32_t Depth;
        uint32_t Normal;
        uint32_t NoiseTexture;
        uint32_t KernelBuffer;
        
        uint32_t CameraBuffer;
        uint32_t KernelSize;
        float Radius;
        float Bias;
        
        uint32_t PointSampler;
        uint32_t PointClampSampler;
        uint32_t Output;
        uint32_t Power;
    };
    Data data = {
        _depth->SRV(),
        _normals->SRV(),
        _noise->SRV(),
        _kernelBuffer->CBV(),
        _cameraBuffer[frameIndex]->CBV(),
        _kernelSize,
        _radius,
        _bias,
        _pointSampler->BindlesssSampler(),
        _pointClampSampler->BindlesssSampler(),
        _ssao->UAV(),
        _power
    };

    commandBuffer->BeginEvent("SSAO Generation");
    commandBuffer->ImageBarrierBatch({
        { _depth, TextureLayout::ShaderResource },
        { _normals, TextureLayout::ShaderResource },
        { _noise, TextureLayout::ShaderResource },
        { _ssao, TextureLayout::Storage }
    });
    commandBuffer->BindComputePipeline(_ssaoPipeline.ComputePipeline);
    commandBuffer->PushConstantsCompute(&data, sizeof(data), 0);
    commandBuffer->Dispatch(std::ceil(width / 8), std::ceil(height / 8), 1);
    commandBuffer->ImageBarrierBatch({
        { _ssao, TextureLayout::Storage },
        { _depth, TextureLayout::Depth }
    });
    commandBuffer->EndEvent();
}

void SSAO::BlurPass(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();

    struct Data {
        uint32_t Output;
        uint32_t BlurSize;
        glm::uvec2 Pad;
    };
    Data data = {
        _ssao->UAV(),
        _blurSize,
        glm::uvec2(0u)
    };

    commandBuffer->BeginEvent("SSAO Blur");
    commandBuffer->ImageBarrierBatch({
        { _ssao, TextureLayout::Storage }
    });
    commandBuffer->BindComputePipeline(_ssaoBlur.ComputePipeline);
    commandBuffer->PushConstantsCompute(&data, sizeof(data), 0);
    commandBuffer->Dispatch(std::ceil(width / 8), std::ceil(height / 8), 1);
    commandBuffer->ImageBarrierBatch({
        { _ssao, TextureLayout::ShaderResource }
    });
    commandBuffer->EndEvent();
}

void SSAO::Resize(uint32_t width, uint32_t height)
{

}

void SSAO::OnUI()
{
    if (ImGui::TreeNodeEx("SSAO", ImGuiTreeNodeFlags_Framed)) {
        ImGui::SliderInt("Strength", reinterpret_cast<int*>(&_power), 0, 10);
        ImGui::SliderInt("Kernel Size", reinterpret_cast<int*>(&_kernelSize), 0, 64);
        ImGui::TreePop();
    }
}

void SSAO::Reconstruct()
{
    _ssaoPipeline.CheckForRebuild(_context, "SSAO");
    _ssaoBlur.CheckForRebuild(_context, "SSAO Blur");
}
