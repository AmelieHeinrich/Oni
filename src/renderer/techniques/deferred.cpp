//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-19 13:21:23
//

#include "deferred.hpp"

Deferred::Deferred(RenderContext::Ptr context)
    : _context(context), _gbufferPipeline(PipelineType::Graphics), _lightingPipeline(PipelineType::Compute)
{
    uint32_t width, height;
    context->GetWindow()->GetSize(width, height);

    _normals = _context->CreateTexture(width, height, TextureFormat::RGBA16Float, TextureUsage::RenderTarget, false, "[GBUFFER] Normals");
    _normals->BuildRenderTarget();
    _normals->BuildShaderResource();

    _albedoEmission = _context->CreateTexture(width, height, TextureFormat::RGBA8, TextureUsage::RenderTarget, false, "[GBUFFER] Albedo Emission");
    _albedoEmission->BuildRenderTarget();
    _albedoEmission->BuildShaderResource();

    _pbrData = _context->CreateTexture(width, height, TextureFormat::RGBA8, TextureUsage::RenderTarget, false, "[GBUFFER] PBR + AO");
    _pbrData->BuildRenderTarget();
    _pbrData->BuildShaderResource();

    _whiteTexture = context->CreateTexture(1, 1, TextureFormat::RGBA8, TextureUsage::ShaderResource, false, "White Texture");
    _whiteTexture->BuildShaderResource();

    _blackTexture = context->CreateTexture(1, 1, TextureFormat::RGBA8, TextureUsage::ShaderResource, false, "Black Texture");
    _blackTexture->BuildShaderResource();

    Uploader uploader = context->CreateUploader();

    uint32_t whiteColor = 0xFFFFFFFF;
    Bitmap whiteImage;
    whiteImage.Width = 1;
    whiteImage.Height = 1;
    whiteImage.Delete = false;
    whiteImage.Bytes = reinterpret_cast<char*>(&whiteColor);
    uploader.CopyHostToDeviceTexture(whiteImage, _whiteTexture);

    uint32_t blackColor = 0xFF000000;
    Bitmap blackImage;
    blackImage.Width = 1;
    blackImage.Height = 1;
    blackImage.Delete = false;
    blackImage.Bytes = reinterpret_cast<char*>(&blackColor);
    uploader.CopyHostToDeviceTexture(blackImage, _blackTexture);

    _context->FlushUploader(uploader);

    _outputImage = context->CreateTexture(width, height, TextureFormat::RGBA16Unorm, TextureUsage::RenderTarget, false, "Deferred Output");
    _outputImage->BuildRenderTarget();
    _outputImage->BuildShaderResource();
    _outputImage->BuildStorage();

    _depthBuffer = context->CreateTexture(width, height, TextureFormat::R32Typeless, TextureUsage::DepthTarget, false, "Deferred Depth Buffer");
    _depthBuffer->BuildDepthTarget(TextureFormat::R32Depth);
    _depthBuffer->BuildShaderResource(TextureFormat::R32Float);

    {
        _gbufferPipeline.Specs.FormatCount = 3;
        _gbufferPipeline.Specs.Formats[0] = TextureFormat::RGBA16Float;
        _gbufferPipeline.Specs.Formats[1] = TextureFormat::RGBA8;
        _gbufferPipeline.Specs.Formats[2] = TextureFormat::RGBA8;
        _gbufferPipeline.Specs.DepthFormat = TextureFormat::R32Depth;
        _gbufferPipeline.Specs.Depth = DepthOperation::Less;
        _gbufferPipeline.Specs.DepthEnabled = true;
        _gbufferPipeline.Specs.Cull = CullMode::Front;
        _gbufferPipeline.Specs.Fill = FillMode::Solid;

        _gbufferPipeline.AddShaderWatch("shaders/Deferred/GBuffer/GBufferVert.hlsl", "Main", ShaderType::Vertex);
        _gbufferPipeline.AddShaderWatch("shaders/Deferred/GBuffer/GBufferFrag.hlsl", "Main", ShaderType::Fragment);
        _gbufferPipeline.Build(context);
    }
    
    {
        _lightingPipeline.AddShaderWatch("shaders/Deferred/Lighting/LightingCompute.hlsl", "Main", ShaderType::Compute);
        _lightingPipeline.Build(context);
    }

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        _sceneBufferGPass[i] = context->CreateBuffer(512, 0, BufferType::Constant, false, "Scene Buffer CBV");
        _sceneBufferGPass[i]->BuildConstantBuffer();

        _sceneBufferLight[i] = context->CreateBuffer(1024, 0, BufferType::Constant, false, "Scene Buffer CBV");
        _sceneBufferLight[i]->BuildConstantBuffer();
    
        _lightBuffer[i] = context->CreateBuffer(24832, 0, BufferType::Constant, false, "Light Buffer CBV");
        _lightBuffer[i]->BuildConstantBuffer();

        _modeBuffer[i] = context->CreateBuffer(256, 0, BufferType::Constant, false, "Mode Buffer CBV");
        _modeBuffer[i]->BuildConstantBuffer();
    }

    _sampler = context->CreateSampler(SamplerAddress::Wrap, SamplerFilter::Linear, true, 0);
    _shadowSampler = context->CreateSampler(SamplerAddress::Clamp, SamplerFilter::Linear, false, 0);
}

Deferred::~Deferred()
{

}

void Deferred::GBufferPass(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(commandBuffer->GetCommandList());
    OPTICK_GPU_EVENT("Construct GBuffer");

    glm::mat4 depthProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 0.05f, 50.0f);
    glm::mat4 depthView = glm::lookAt(scene.Lights.SunTransform.Position, scene.Lights.SunTransform.Position - scene.Lights.SunTransform.GetFrontVector(), glm::vec3(0.0f, 1.0f, 0.0f));

    struct Data {
        glm::mat4 CameraMatrix;
    };
    Data data;
    if (!_visualizeShadow) {
        data.CameraMatrix = scene.Camera.Projection() * scene.Camera.View();
    } else {
        data.CameraMatrix = depthProjection * depthView;
    }

    void *pData;
    _sceneBufferGPass[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &data, sizeof(Data));
    _sceneBufferGPass[frameIndex]->Unmap(0, 0);

    _lightBuffer[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &scene.Lights.GetGPUData(), sizeof(LightSettings::GPUData));
    _lightBuffer[frameIndex]->Unmap(0, 0);

    glm::ivec4 mode(_mode, _ibl, 0, 0);
    _modeBuffer[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, glm::value_ptr(mode), sizeof(glm::ivec4));
    _modeBuffer[frameIndex]->Unmap(0, 0);

    commandBuffer->BeginEvent("GBuffer");
    commandBuffer->ImageBarrier(_normals, TextureLayout::RenderTarget);
    commandBuffer->ImageBarrier(_albedoEmission, TextureLayout::RenderTarget);
    commandBuffer->ImageBarrier(_pbrData, TextureLayout::RenderTarget);
    commandBuffer->ClearRenderTarget(_normals, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearRenderTarget(_albedoEmission, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearRenderTarget(_pbrData, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearDepthTarget(_depthBuffer);
    if (_draw) {
        commandBuffer->SetViewport(0, 0, width, height);
        commandBuffer->SetTopology(Topology::TriangleList);
        commandBuffer->BindRenderTargets({ _normals, _albedoEmission, _pbrData }, _depthBuffer);
        commandBuffer->BindGraphicsPipeline(_gbufferPipeline.GraphicsPipeline);
        commandBuffer->BindGraphicsConstantBuffer(_sceneBufferGPass[frameIndex], 0);
        commandBuffer->BindGraphicsSampler(_sampler, 7);

        for (auto model : scene.Models) {
            for (auto primitive : model.Primitives) {
                auto material = model.Materials[primitive.MaterialIndex];

                Texture::Ptr albedo = material.HasAlbedo ? material.AlbedoTexture : _whiteTexture;
                Texture::Ptr normal = material.HasNormal ? material.NormalTexture : _whiteTexture;
                Texture::Ptr pbr = material.HasMetallicRoughness ? material.PBRTexture : _whiteTexture;
                Texture::Ptr emissive = material.HasEmissive ? material.EmissiveTexture : _blackTexture;
                Texture::Ptr ao = material.HasAO ? material.AOTexture : _whiteTexture;

                commandBuffer->BindVertexBuffer(primitive.VertexBuffer);
                commandBuffer->BindIndexBuffer(primitive.IndexBuffer);
                commandBuffer->BindGraphicsConstantBuffer(primitive.ModelBuffer, 1);
                commandBuffer->BindGraphicsShaderResource(albedo, 2);
                commandBuffer->BindGraphicsShaderResource(normal, 3);
                commandBuffer->BindGraphicsShaderResource(pbr, 4);
                commandBuffer->BindGraphicsShaderResource(emissive, 5);
                commandBuffer->BindGraphicsShaderResource(ao, 6);
                commandBuffer->DrawIndexed(primitive.IndexCount);
            }
        }
    }
    commandBuffer->EndEvent();
}

void Deferred::LightingPass(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(commandBuffer->GetCommandList());
    OPTICK_GPU_EVENT("Deferred Lighting");

    glm::mat4 depthProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 0.05f, 50.0f);
    glm::mat4 depthView = glm::lookAt(scene.Lights.SunTransform.Position, scene.Lights.SunTransform.Position - scene.Lights.SunTransform.GetFrontVector(), glm::vec3(0.0f, 1.0f, 0.0f));

    struct Data {
        glm::mat4 CameraMatrix;
        glm::mat4 CameraProjViewInv;
        glm::mat4 LightMatrix;
        glm::vec4 CameraPosition;
    };
    Data data;
    data.CameraMatrix = scene.Camera.Projection() * scene.Camera.View();
    data.CameraProjViewInv = glm::inverse(scene.Camera.Projection() * scene.Camera.View());
    data.LightMatrix = depthProjection * depthView;
    data.CameraPosition = glm::vec4(scene.Camera.GetPosition(), 1.0f);

    void *pData;
    _sceneBufferGPass[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &data, sizeof(Data));
    _sceneBufferGPass[frameIndex]->Unmap(0, 0);

    commandBuffer->BeginEvent("Deferred Lighting");
    commandBuffer->ImageBarrier(_outputImage, TextureLayout::Storage);
    if (_draw) {
        commandBuffer->SetViewport(0, 0, width, height);

        commandBuffer->BindComputePipeline(_lightingPipeline.ComputePipeline);

        commandBuffer->BindComputeShaderResource(_depthBuffer, 0);
        commandBuffer->BindComputeShaderResource(_normals, 1);
        commandBuffer->BindComputeShaderResource(_albedoEmission, 2);
        commandBuffer->BindComputeShaderResource(_pbrData, 3);
        commandBuffer->BindComputeCubeMapShaderResource(_map.IrradianceMap, 4);
        commandBuffer->BindComputeCubeMapShaderResource(_map.PrefilterMap, 5);
        commandBuffer->BindComputeShaderResource(_map.BRDF, 6);
        commandBuffer->BindComputeShaderResource(_shadowMap, 7);
        commandBuffer->BindComputeSampler(_sampler, 8),
        commandBuffer->BindComputeSampler(_shadowSampler, 9);
        commandBuffer->BindComputeConstantBuffer(_sceneBufferGPass[frameIndex], 10);
        commandBuffer->BindComputeConstantBuffer(_lightBuffer[frameIndex], 11);
        commandBuffer->BindComputeConstantBuffer(_modeBuffer[frameIndex], 12);
        commandBuffer->BindComputeStorageTexture(_outputImage, 13);
        
        commandBuffer->Dispatch(width / 8, height / 8, 1);
    }
    commandBuffer->EndEvent();
}

void Deferred::Render(Scene& scene, uint32_t width, uint32_t height)
{
    GBufferPass(scene, width, height);
    LightingPass(scene, width, height);
}

void Deferred::Resize(uint32_t width, uint32_t height)
{
    _normals.reset();
    _albedoEmission.reset();
    _pbrData.reset();
    _outputImage.reset();
    _depthBuffer.reset();

    _normals = _context->CreateTexture(width, height, TextureFormat::RGBA16Float, TextureUsage::RenderTarget, false, "[GBUFFER] Normals");
    _normals->BuildRenderTarget();
    _normals->BuildShaderResource();

    _albedoEmission = _context->CreateTexture(width, height, TextureFormat::RGBA8, TextureUsage::RenderTarget, false, "[GBUFFER] Albedo Emission");
    _albedoEmission->BuildRenderTarget();
    _albedoEmission->BuildShaderResource();

    _pbrData = _context->CreateTexture(width, height, TextureFormat::RGBA8, TextureUsage::RenderTarget, false, "[GBUFFER] PBR + AO");
    _pbrData->BuildRenderTarget();
    _pbrData->BuildShaderResource();

    _outputImage = _context->CreateTexture(width, height, TextureFormat::RGBA16Unorm, TextureUsage::RenderTarget, false, "Forward RTV");
    _outputImage->BuildRenderTarget();
    _outputImage->BuildShaderResource();

    _depthBuffer = _context->CreateTexture(width, height, TextureFormat::R32Typeless, TextureUsage::DepthTarget, false, "Deferred Depth Buffer");
    _depthBuffer->BuildDepthTarget(TextureFormat::R32Depth);
    _depthBuffer->BuildShaderResource(TextureFormat::R32Float);
}

void Deferred::OnUI()
{
    if (ImGui::TreeNodeEx("Deferred", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Draw Geometry", &_draw);
        ImGui::Checkbox("Enable IBL", &_ibl);
        ImGui::Checkbox("Visualize Shadows", &_visualizeShadow);

        static const char* Modes[] = { "Default", "Albedo", "Normal", "Metallic Roughness", "AO", "Emissive", "Specular", "Ambient", "Position" };
        ImGui::Combo("Mode", (int*)&_mode, Modes, 9, 9);

        ImGui::TreePop();
    }
}

void Deferred::Reconstruct()
{
    _gbufferPipeline.CheckForRebuild(_context);
    _lightingPipeline.CheckForRebuild(_context);
}

void Deferred::ConnectEnvironmentMap(EnvironmentMap& map)
{
    _map = map;
}

void Deferred::ConnectShadowMap(Texture::Ptr texture)
{
    _shadowMap = texture;
}
