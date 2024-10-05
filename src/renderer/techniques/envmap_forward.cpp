/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 01:21:55
 */

#include "envmap_forward.hpp"

#include <core/log.hpp>
#include <core/shader_loader.hpp>
#include <optick.h>

#include <ImGui/imgui.h>

const float CubeVertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    
    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
    
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    
    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
    
    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,
    
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

EnvMapForward::EnvMapForward(RenderContext::Ptr context, Texture::Ptr inputColor, Texture::Ptr inputDepth)
    : _context(context), _inputColor(inputColor), _inputDepth(inputDepth), _cubeRenderer(PipelineType::Graphics)
{
    ShaderBytecode cubemapBytecode = ShaderLoader::GetFromCache("shaders/EquiMap/EquiMapCompute.hlsl");
    ShaderBytecode irradianceBytecode = ShaderLoader::GetFromCache("shaders/Irradiance/IrradianceCompute.hlsl");
    ShaderBytecode prefilterBytecode = ShaderLoader::GetFromCache("shaders/Prefilter/PrefilterCompute.hlsl");
    ShaderBytecode brdfBytecode = ShaderLoader::GetFromCache("shaders/BRDF/BRDFCompute.hlsl");

    // Create pipelines
    _envToCube = context->CreateComputePipeline(cubemapBytecode, _context->CreateDefaultRootSignature(sizeof(glm::ivec3)));
    _irradiance = context->CreateComputePipeline(irradianceBytecode, _context->CreateDefaultRootSignature(sizeof(glm::ivec3)));
    _prefilter = context->CreateComputePipeline(prefilterBytecode, _context->CreateDefaultRootSignature(sizeof(glm::vec4)));
    _brdf = context->CreateComputePipeline(brdfBytecode, _context->CreateDefaultRootSignature(sizeof(uint32_t)));

    _cubeRenderer.Specs.Fill = FillMode::Solid;
    _cubeRenderer.Specs.Cull = CullMode::None;
    _cubeRenderer.Specs.DepthEnabled = true;
    _cubeRenderer.Specs.Depth = DepthOperation::LEqual;
    _cubeRenderer.Specs.DepthFormat = TextureFormat::R32Depth;
    _cubeRenderer.Specs.Formats[0] = TextureFormat::RGBA16Unorm;
    _cubeRenderer.Specs.FormatCount = 1;
    
    _cubeRenderer.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(glm::ivec4) + sizeof(glm::mat4)
    };

    _cubeRenderer.ReflectRootSignature(false);
    _cubeRenderer.AddShaderWatch("shaders/EnvMapForward/EnvMapForwardVert.hlsl", "Main", ShaderType::Vertex);
    _cubeRenderer.AddShaderWatch("shaders/EnvMapForward/EnvMapForwardFrag.hlsl", "Main", ShaderType::Fragment);
    _cubeRenderer.Build(_context);

    // Create sampler
    _cubeSampler = context->CreateSampler(SamplerAddress::Wrap, SamplerFilter::Linear, false, 0);

    // Load HDRI
    Bitmap image;
    image.LoadHDR("assets/env/day/newport_loft.hdr");

    Texture::Ptr hdrTexture = context->CreateTexture(image.Width, image.Height, TextureFormat::RGBA16Unorm, TextureUsage::ShaderResource, false, "HDR Texture");
    hdrTexture->BuildShaderResource();

    _cubeBuffer = context->CreateBuffer(sizeof(CubeVertices), sizeof(glm::vec3), BufferType::Vertex, false, "[ENVMAP] Cube Buffer");

    Uploader uploader = context->CreateUploader();
    uploader.CopyHostToDeviceTexture(image, hdrTexture);
    uploader.CopyHostToDeviceLocal((void*)CubeVertices, sizeof(CubeVertices), _cubeBuffer);
    context->FlushUploader(uploader);

    // Create textures
    _map.Environment = context->CreateCubeMap(512, 512, TextureFormat::RGBA16Float, 1, "[ENVMAP] Environment Map");
    _map.IrradianceMap = context->CreateCubeMap(32, 32, TextureFormat::RGBA16Float, 1, "[ENVMAP] Irradiance Map");
    _map.PrefilterMap = context->CreateCubeMap(512, 512, TextureFormat::RGBA16Float, 5, "[ENVMAP] Prefilter Map");
    _map.BRDF = context->CreateTexture(512, 512, TextureFormat::RG16Float, TextureUsage::Storage, false, "[ENVMAP] BRDF");
    _map.BRDF->BuildShaderResource();
    _map.BRDF->BuildStorage();

    float startTime = clock();

    CommandBuffer::Ptr cmdBuffer = context->CreateCommandBuffer(CommandQueueType::Graphics, false);

    // Equi to cubemap
    cmdBuffer->Begin(false);
    cmdBuffer->BindComputePipeline(_envToCube);
    cmdBuffer->PushConstantsCompute(glm::value_ptr(glm::ivec3(hdrTexture->SRV(), _map.Environment->UAV(), _cubeSampler->BindlesssSampler())), sizeof(glm::ivec3), 0);
    cmdBuffer->Dispatch(512 / 32, 512 / 32, 6);

    // Irradiance
    cmdBuffer->BindComputePipeline(_irradiance);
    cmdBuffer->PushConstantsCompute(glm::value_ptr(glm::ivec3(_map.Environment->SRV(), _map.IrradianceMap->UAV(0), _cubeSampler->BindlesssSampler())), sizeof(glm::ivec3), 0);
    cmdBuffer->Dispatch(32 / 32, 32 / 32, 6); 

    // Prefilter
    const float deltaRoughness = 1.0f / std::max(float(_map.PrefilterMap->GetMips() - 1u), 1.0f);
    cmdBuffer->BindComputePipeline(_prefilter);
    for (int level = 0, size = 512; level < _map.PrefilterMap->GetMips(); ++level, size /= 2) {
        const uint32_t numGroups = std::max(1u, size / 32u);

        struct PushConstant {
            uint32_t EnvMap;
            uint32_t PrefilterMap;
            uint32_t Sampler;
            float Roughness;
        };
        PushConstant constants = {
            _map.Environment->SRV(),
            _map.PrefilterMap->UAV(level),
            _cubeSampler->BindlesssSampler(),
            level * deltaRoughness
        };

        cmdBuffer->PushConstantsCompute(&constants, sizeof(constants), 0);
        cmdBuffer->Dispatch(numGroups, numGroups, 6);
    }

    // BRDF
    uint32_t lut = _map.BRDF->UAV();
    cmdBuffer->BindComputePipeline(_brdf);
    cmdBuffer->PushConstantsCompute(&lut, sizeof(uint32_t), 0);
    cmdBuffer->Dispatch(512 / 32, 512 / 32, 1);
    cmdBuffer->ImageBarrier(_map.BRDF, TextureLayout::ShaderResource);

    cmdBuffer->End();
    _context->ExecuteCommandBuffers({ cmdBuffer }, CommandQueueType::Graphics);
    _context->WaitForGPU();

    float endTime = (clock() - startTime) / 1000.0f;
    Logger::Info("[ENVMAP] Environment map: Texture generation took %f seconds", endTime);
}

EnvMapForward::~EnvMapForward()
{

}

void EnvMapForward::Render(Scene& scene, uint32_t width, uint32_t height)
{
    glm::mat4 mvp = scene.Camera.Projection() * glm::mat4(glm::mat3(scene.Camera.View())) * glm::scale(glm::mat4(1.0f), glm::vec3(1000.0f));

    CommandBuffer::Ptr cmdBuffer = _context->GetCurrentCommandBuffer();

    OPTICK_GPU_CONTEXT(cmdBuffer->GetCommandList());
    OPTICK_GPU_EVENT("Cube Map Forward");

    if (_drawSkybox) {
        struct Constants {
            uint32_t Environment;
            uint32_t Sampler;
            glm::vec2 _Pad0;
            glm::mat4 MVP;
        };
        Constants constants = {
            _map.Environment->SRV(),
            _cubeSampler->BindlesssSampler(),
            glm::vec2(0.0f),
            mvp
        };

        cmdBuffer->BeginEvent("Draw Skybox");
        cmdBuffer->SetViewport(0, 0, width, height);
        cmdBuffer->SetTopology(Topology::TriangleList);
        cmdBuffer->ImageBarrier(_inputColor, TextureLayout::RenderTarget);
        cmdBuffer->BindRenderTargets({ _inputColor }, _inputDepth);
        cmdBuffer->BindGraphicsPipeline(_cubeRenderer.GraphicsPipeline);
        cmdBuffer->PushConstantsGraphics(&constants, sizeof(constants), 0);
        cmdBuffer->BindVertexBuffer(_cubeBuffer);
        cmdBuffer->Draw(36);
        cmdBuffer->EndEvent();
    }
}

void EnvMapForward::Resize(uint32_t width, uint32_t height, Texture::Ptr inputColor, Texture::Ptr inputDepth)
{
    _inputColor = inputColor;
    _inputDepth = inputDepth;
}

void EnvMapForward::OnUI()
{
    if (ImGui::TreeNodeEx("Environment Map", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Draw Skybox", &_drawSkybox);
        ImGui::TreePop();
    }
}

void EnvMapForward::Reconstruct()
{
    _cubeRenderer.CheckForRebuild(_context, "Environment Map");
}
