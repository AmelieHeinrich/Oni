/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 01:21:55
 */

#include "envmap_forward.hpp"

#include <core/log.hpp>

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
    : _context(context), _inputColor(inputColor), _inputDepth(inputDepth)
{
    ShaderBytecode cubemapBytecode, prefilterBytecode, irradianceBytecode, brdfBytecode;
    ShaderCompiler::CompileShader("shaders/EquiMap/EquiMapCompute.hlsl", "Main", ShaderType::Compute, cubemapBytecode);
    ShaderCompiler::CompileShader("shaders/Irradiance/IrradianceCompute.hlsl", "Main", ShaderType::Compute, irradianceBytecode);
    ShaderCompiler::CompileShader("shaders/Prefilter/PrefilterCompute.hlsl", "Main", ShaderType::Compute, prefilterBytecode);
    ShaderCompiler::CompileShader("shaders/BRDF/BRDFCompute.hlsl", "Main", ShaderType::Compute, brdfBytecode);

    ShaderBytecode cubemapRendererVertex, cubemapRendererFragment;
    ShaderCompiler::CompileShader("shaders/EnvMapForward/EnvMapForwardVert.hlsl", "Main", ShaderType::Vertex, cubemapRendererVertex);
    ShaderCompiler::CompileShader("shaders/EnvMapForward/EnvMapForwardFrag.hlsl", "Main", ShaderType::Fragment, cubemapRendererFragment);

    // Create pipelines
    Uploader uploader = context->CreateUploader();

    _envToCube = context->CreateComputePipeline(cubemapBytecode);
    _irradiance = context->CreateComputePipeline(irradianceBytecode);
    _prefilter = context->CreateComputePipeline(prefilterBytecode);
    _brdf = context->CreateComputePipeline(brdfBytecode);

    GraphicsPipelineSpecs specs;
    specs.Bytecodes[ShaderType::Vertex] = cubemapRendererVertex;
    specs.Bytecodes[ShaderType::Fragment] = cubemapRendererFragment;
    specs.Fill = FillMode::Solid;
    specs.Cull = CullMode::None;
    specs.DepthEnabled = true;
    specs.Depth = DepthOperation::LEqual;
    specs.DepthFormat = TextureFormat::R32Depth;
    specs.Formats[0] = TextureFormat::RGBA16Unorm;
    specs.FormatCount = 1;
    _cubeRenderer = context->CreateGraphicsPipeline(specs);

    // Create sampler
    _cubeSampler = context->CreateSampler(SamplerAddress::Wrap, SamplerFilter::Linear, 0);

    // Load HDRI
    Image image;
    image.LoadHDR("assets/env/symmetrical_garden_02_4k.hdr");

    Texture::Ptr hdrTexture = context->CreateTexture(image.Width, image.Height, TextureFormat::RGBA16Unorm, TextureUsage::ShaderResource, "HDR Texture");
    hdrTexture->BuildShaderResource();

    uploader.CopyHostToDeviceTexture(image, hdrTexture);

    // Create textures
    _map.Environment = context->CreateCubeMap(512, 512, TextureFormat::RGBA16Unorm, "Environment Map");
    _map.IrradianceMap = context->CreateCubeMap(128, 128, TextureFormat::RGBA16Unorm, "Irradiance Map");
    _map.PrefilterMap = context->CreateCubeMap(512, 512, TextureFormat::RGBA16Unorm, "Prefilter Map");
    _map.BRDF = context->CreateTexture(512, 512, TextureFormat::RG16Float, TextureUsage::Storage, "BRDF");
    _map.BRDF->BuildShaderResource();
    _map.BRDF->BuildStorage();

    // Create geometry
    _cubeBuffer = context->CreateBuffer(sizeof(CubeVertices), sizeof(glm::vec3), BufferType::Vertex, false, "Cube Buffer");
    _cubeCBV = context->CreateBuffer(256, 0, BufferType::Constant, false, "Cube Map CBV");
    _cubeCBV->BuildConstantBuffer();
    _prefilterBuffer = context->CreateBuffer(256, 0, BufferType::Constant, false, "Prefilter Buffer CBV");
    _prefilterBuffer->BuildConstantBuffer();

    uploader.CopyHostToDeviceLocal((void*)CubeVertices, sizeof(CubeVertices), _cubeBuffer);

    // Run everything
    context->FlushUploader(uploader);

    float startTime = clock();

    CommandBuffer::Ptr cmdBuffer = context->CreateCommandBuffer(CommandQueueType::Graphics);

    // Equi to cubemap
    cmdBuffer->Begin();
    cmdBuffer->BindComputePipeline(_envToCube);
    cmdBuffer->BindComputeShaderResource(hdrTexture, 0);
    cmdBuffer->BindComputeCubeMapStorage(_map.Environment, 1);
    cmdBuffer->BindComputeSampler(_cubeSampler, 2);
    cmdBuffer->Dispatch(512 / 32, 512 / 32, 6);
    cmdBuffer->End();
    _context->ExecuteCommandBuffers({ cmdBuffer }, CommandQueueType::Graphics);

    // Irradiance
    cmdBuffer->Begin();
    cmdBuffer->BindComputePipeline(_irradiance);
    cmdBuffer->BindComputeCubeMapShaderResource(_map.Environment, 0);
    cmdBuffer->BindComputeCubeMapStorage(_map.IrradianceMap, 1);
    cmdBuffer->BindComputeSampler(_cubeSampler, 2);
    cmdBuffer->Dispatch(128 / 32, 128 / 32, 6);
    cmdBuffer->End();
    _context->ExecuteCommandBuffers({ cmdBuffer }, CommandQueueType::Graphics);

    // Prefilter
    cmdBuffer->Begin();
    cmdBuffer->BindComputePipeline(_prefilter);
    cmdBuffer->BindComputeCubeMapShaderResource(_map.Environment, 0);
    cmdBuffer->BindComputeCubeMapStorage(_map.PrefilterMap, 1);
    cmdBuffer->BindComputeSampler(_cubeSampler, 2);
    for (int i = 0; i < 5; i++) {
        int width = (int)(512.0f * pow(0.5f, i));
        int height = (int)(512.0f * pow(0.5f, i));
        float roughness = (float)i / (float)(5 - 1);

        glm::vec4 roughnessVec(roughness, 0.0f, 0.0f, 0.0f);

        void *pData;
        _prefilterBuffer->Map(0, 0, &pData);
        memcpy(pData, glm::value_ptr(roughnessVec), sizeof(glm::vec4));
        _prefilterBuffer->Unmap(0, 0);

        cmdBuffer->BindComputeConstantBuffer(_prefilterBuffer, 3);
        cmdBuffer->Dispatch(width / 32, height / 32, 6);
    }
    cmdBuffer->End();
    _context->ExecuteCommandBuffers({ cmdBuffer }, CommandQueueType::Graphics);

    // BRDF
    cmdBuffer->Begin();
    cmdBuffer->BindComputePipeline(_brdf);
    cmdBuffer->BindComputeStorageTexture(_map.BRDF, 0);
    cmdBuffer->Dispatch(512 / 32, 512 / 32, 1);
    cmdBuffer->ImageBarrier(_map.BRDF, TextureLayout::ShaderResource);
    cmdBuffer->End();
    _context->ExecuteCommandBuffers({ cmdBuffer }, CommandQueueType::Graphics);

    float endTime = (clock() - startTime) / 1000.0f;
    Logger::Info("Environment map: Texture generation took %f seconds", endTime);
}

EnvMapForward::~EnvMapForward()
{

}

void EnvMapForward::Render(Scene& scene, uint32_t width, uint32_t height)
{
    glm::mat4 mvp = scene.Projection * glm::mat4(glm::mat3x3(scene.View)) * glm::scale(glm::mat4(1.0f), glm::vec3(1000.0f));

    void *pData;
    _cubeCBV->Map(0, 0, &pData);
    memcpy(pData, glm::value_ptr(mvp), sizeof(glm::mat4));
    _cubeCBV->Unmap(0, 0);

    CommandBuffer::Ptr cmdBuffer = _context->GetCurrentCommandBuffer();

    cmdBuffer->Begin();
    cmdBuffer->SetViewport(0, 0, width, height);
    cmdBuffer->SetTopology(Topology::TriangleList);
    cmdBuffer->BindRenderTargets({ _inputColor }, _inputDepth);
    cmdBuffer->BindGraphicsPipeline(_cubeRenderer);
    cmdBuffer->BindGraphicsConstantBuffer(_cubeCBV, 0);
    cmdBuffer->BindGraphicsCubeMap(_map.Environment, 1);
    cmdBuffer->BindGraphicsSampler(_cubeSampler, 2);
    cmdBuffer->BindVertexBuffer(_cubeBuffer);
    cmdBuffer->Draw(36);
    cmdBuffer->End();
    _context->ExecuteCommandBuffers({ cmdBuffer }, CommandQueueType::Graphics);
}

void EnvMapForward::Resize(uint32_t width, uint32_t height, Texture::Ptr inputColor, Texture::Ptr inputDepth)
{
    _inputColor = inputColor;
    _inputDepth = inputDepth;
}

void EnvMapForward::OnUI()
{

}
