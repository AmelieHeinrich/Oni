/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 01:21:55
 */

#include "envmap_forward.hpp"

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

    ShaderBytecode cubemapRendererVertex, cubemapRendererFragment;
    ShaderCompiler::CompileShader("shaders/EnvMapForward/EnvMapForwardVert.hlsl", "Main", ShaderType::Vertex, cubemapRendererVertex);
    ShaderCompiler::CompileShader("shaders/EnvMapForward/EnvMapForwardFrag.hlsl", "Main", ShaderType::Fragment, cubemapRendererFragment);

    // Create pipelines
    Uploader uploader = context->CreateUploader();

    _envToCube = context->CreateComputePipeline(cubemapBytecode);

    GraphicsPipelineSpecs specs;
    specs.Bytecodes[ShaderType::Vertex] = cubemapRendererVertex;
    specs.Bytecodes[ShaderType::Fragment] = cubemapRendererFragment;
    specs.Fill = FillMode::Solid;
    specs.Cull = CullMode::None;
    specs.DepthEnabled = true;
    specs.Depth = DepthOperation::LEqual;
    specs.DepthFormat = TextureFormat::R32Depth;
    specs.Formats[0] = TextureFormat::RGBA32Float;
    specs.FormatCount = 1;
    _cubeRenderer = context->CreateGraphicsPipeline(specs);

    // Create sampler
    _cubeSampler = context->CreateSampler(SamplerAddress::Wrap, SamplerFilter::Nearest, 0);

    // Load HDRI
    Image image;
    image.LoadHDR("assets/env/env_map.hdr");

    Texture::Ptr hdrTexture = context->CreateTexture(image.Width, image.Height, TextureFormat::RGBA32Float, TextureUsage::ShaderResource);
    hdrTexture->BuildShaderResource();

    uploader.CopyHostToDeviceTexture(image, hdrTexture);

    // Create textures
    _map.Environment = context->CreateCubeMap(512, 512, TextureFormat::RGBA32Float);

    // Create geometry
    _cubeBuffer = context->CreateBuffer(sizeof(CubeVertices), sizeof(glm::vec3), BufferType::Vertex, false);
    _cubeCBV = context->CreateBuffer(256, 0, BufferType::Constant, false);
    _cubeCBV->BuildConstantBuffer();

    uploader.CopyHostToDeviceLocal((void*)CubeVertices, sizeof(CubeVertices), _cubeBuffer);

    // Run everything
    context->FlushUploader(uploader);

    CommandBuffer::Ptr cmdBuffer = context->CreateCommandBuffer(CommandQueueType::Graphics);

    cmdBuffer->Begin();
    cmdBuffer->CubeMapBarrier(_map.Environment, TextureLayout::Storage);
    cmdBuffer->BindComputePipeline(_envToCube);
    cmdBuffer->BindComputeShaderResource(hdrTexture, 0);
    cmdBuffer->BindComputeCubeMapStorage(_map.Environment, 1);
    cmdBuffer->BindComputeSampler(_cubeSampler, 2);
    cmdBuffer->Dispatch(512 / 32, 512 / 32, 6);
    cmdBuffer->CubeMapBarrier(_map.Environment, TextureLayout::ShaderResource);
    cmdBuffer->End();
    _context->ExecuteCommandBuffers({ cmdBuffer }, CommandQueueType::Graphics);
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
