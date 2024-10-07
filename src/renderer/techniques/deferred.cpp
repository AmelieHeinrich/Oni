//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-19 13:21:23
//

#include "deferred.hpp"

#include "core/log.hpp"

Deferred::Deferred(RenderContext::Ptr context)
    : _context(context), _gbufferPipeline(PipelineType::Graphics), _gbufferPipelineMesh(PipelineType::Mesh), _lightingPipeline(PipelineType::Compute)
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

    _velocityBuffer = _context->CreateTexture(width, height, TextureFormat::RG16Float, TextureUsage::RenderTarget, false, "[GBUFFER] Velocity buffer");
    _velocityBuffer->BuildRenderTarget();
    _velocityBuffer->BuildShaderResource();
    
    _emissive = _context->CreateTexture(width, height, TextureFormat::RGBA16Float, TextureUsage::RenderTarget, false, "[GBUFFER] Emissive");
    _emissive->BuildRenderTarget();
    _emissive->BuildShaderResource();

    _whiteTexture = context->CreateTexture(1, 1, TextureFormat::RGBA8, TextureUsage::ShaderResource, false, "[DEFERRED] White Texture");
    _whiteTexture->BuildShaderResource();

    _blackTexture = context->CreateTexture(1, 1, TextureFormat::RGBA8, TextureUsage::ShaderResource, false, "[DEFERRED] Black Texture");
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

    _outputImage = context->CreateTexture(width, height, TextureFormat::RGBA16Unorm, TextureUsage::RenderTarget, false, "[DEFERRED] Deferred Output");
    _outputImage->BuildRenderTarget();
    _outputImage->BuildShaderResource();
    _outputImage->BuildStorage();

    _depthBuffer = context->CreateTexture(width, height, TextureFormat::R32Typeless, TextureUsage::DepthTarget, false, "[DEFERRED] Deferred Depth Buffer");
    _depthBuffer->BuildDepthTarget(TextureFormat::R32Depth);
    _depthBuffer->BuildShaderResource(TextureFormat::R32Float);

    {
        _gbufferPipeline.Specs.FormatCount = 5;
        _gbufferPipeline.Specs.Formats[0] = TextureFormat::RGBA16Float;
        _gbufferPipeline.Specs.Formats[1] = TextureFormat::RGBA8;
        _gbufferPipeline.Specs.Formats[2] = TextureFormat::RGBA8;
        _gbufferPipeline.Specs.Formats[3] = TextureFormat::RGBA16Float;
        _gbufferPipeline.Specs.Formats[4] = TextureFormat::RG16Float;
        _gbufferPipeline.Specs.DepthFormat = TextureFormat::R32Depth;
        _gbufferPipeline.Specs.Depth = DepthOperation::Less;
        _gbufferPipeline.Specs.DepthEnabled = true;
        _gbufferPipeline.Specs.Cull = CullMode::Front;
        _gbufferPipeline.Specs.Fill = FillMode::Solid;
        _gbufferPipeline.Specs.CCW = false;

        _gbufferPipeline.SignatureInfo = {
            { RootSignatureEntry::PushConstants },
            48
        };
        _gbufferPipeline.ReflectRootSignature(false);
        _gbufferPipeline.AddShaderWatch("shaders/Deferred/GBuffer/Classic/GBufferVert.hlsl", "Main", ShaderType::Vertex);
        _gbufferPipeline.AddShaderWatch("shaders/Deferred/GBuffer/Classic/GBufferFrag.hlsl", "Main", ShaderType::Fragment);
        _gbufferPipeline.Build(context);
    }

    {
        _gbufferPipelineMesh.Specs.FormatCount = 5;
        _gbufferPipelineMesh.Specs.Formats[0] = TextureFormat::RGBA16Float;
        _gbufferPipelineMesh.Specs.Formats[1] = TextureFormat::RGBA8;
        _gbufferPipelineMesh.Specs.Formats[2] = TextureFormat::RGBA8;
        _gbufferPipelineMesh.Specs.Formats[3] = TextureFormat::RGBA16Float;
        _gbufferPipelineMesh.Specs.Formats[4] = TextureFormat::RG16Float;
        _gbufferPipelineMesh.Specs.DepthFormat = TextureFormat::R32Depth;
        _gbufferPipelineMesh.Specs.Depth = DepthOperation::Less;
        _gbufferPipelineMesh.Specs.DepthEnabled = true;
        _gbufferPipelineMesh.Specs.Cull = CullMode::Front;
        _gbufferPipelineMesh.Specs.Fill = FillMode::Solid;
        _gbufferPipelineMesh.Specs.CCW = false;

        _gbufferPipelineMesh.SignatureInfo = {
            { RootSignatureEntry::PushConstants },
            15 * sizeof(uint32_t)
        };
        _gbufferPipelineMesh.ReflectRootSignature(false);
        _gbufferPipelineMesh.AddShaderWatch("shaders/Deferred/GBuffer/Mesh/GBufferMesh.hlsl", "Main", ShaderType::Mesh);
        _gbufferPipelineMesh.AddShaderWatch("shaders/Deferred/GBuffer/Mesh/GBufferFrag.hlsl", "Main", ShaderType::Fragment);
        _gbufferPipelineMesh.Build(context);
    }
    
    {
        _lightingPipeline.SignatureInfo = {
            { RootSignatureEntry::PushConstants },
            84
        };
        _lightingPipeline.ReflectRootSignature(false);
        _lightingPipeline.AddShaderWatch("shaders/Deferred/Lighting/LightingCompute.hlsl", "Main", ShaderType::Compute);
        _lightingPipeline.Build(context);
    }

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        _sceneBufferLight[i] = context->CreateBuffer(1024, 0, BufferType::Constant, false, "[DEFERRED] Scene Buffer CBV");
        _sceneBufferLight[i]->BuildConstantBuffer();
    
        _lightBuffer[i] = context->CreateBuffer(24832, 0, BufferType::Constant, false, "[DEFERRED] Light Buffer CBV");
        _lightBuffer[i]->BuildConstantBuffer();

        _modeBuffer[i] = context->CreateBuffer(256, 0, BufferType::Constant, false, "[DEFERRED] Mode Buffer CBV");
        _modeBuffer[i]->BuildConstantBuffer();
    }

    _anisotropicSampler = context->CreateSampler(SamplerAddress::Wrap, SamplerFilter::Anistropic, true, 16);
    _sampler = context->CreateSampler(SamplerAddress::Wrap, SamplerFilter::Linear, true, 0);
    _cubeSampler = context->CreateSampler(SamplerAddress::Clamp, SamplerFilter::Linear, true, 0);
    _shadowSampler = context->CreateSampler(SamplerAddress::Clamp, SamplerFilter::Linear, false, 0);

    // Generate halton sequence
    _haltonSequence = {
        glm::vec2(0.500000f, 0.333333f),
        glm::vec2(0.250000f, 0.666667f),
        glm::vec2(0.750000f, 0.111111f),
        glm::vec2(0.125000f, 0.444444f),
        glm::vec2(0.625000f, 0.777778f),
        glm::vec2(0.375000f, 0.222222f),
        glm::vec2(0.875000f, 0.555556f),
        glm::vec2(0.062500f, 0.888889f),
        glm::vec2(0.562500f, 0.037037f),
        glm::vec2(0.312500f, 0.370370f),
        glm::vec2(0.812500f, 0.703704f),
        glm::vec2(0.187500f, 0.148148f),
        glm::vec2(0.687500f, 0.481481f),
        glm::vec2(0.437500f, 0.814815f),
        glm::vec2(0.937500f, 0.259259f),
        glm::vec2(0.031250f, 0.592593f)
    };
    for (int i = 0; i < _haltonSequence.size(); i++) {
        _haltonSequence[i].x = ((_haltonSequence[i].x - 0.5f) / width) * 2;
        _haltonSequence[i].y = ((_haltonSequence[i].y - 0.5f) / height) * 2;
    }
}

Deferred::~Deferred()
{

}

void Deferred::GBufferPassClassic(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(commandBuffer->GetCommandList());
    OPTICK_GPU_EVENT("Construct GBuffer");

    // Construct matrices
    glm::mat4 depthProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 0.05f, 50.0f);
    glm::mat4 depthView = glm::lookAt(scene.Lights.SunTransform.Position, scene.Lights.SunTransform.Position - scene.Lights.SunTransform.GetFrontVector(), glm::vec3(0.0f, 1.0f, 0.0f));

    // Apply jitter
    _currJitter = _haltonSequence[_jitterCounter];
    _jitterCounter = (_jitterCounter + 1) % (_haltonSequence.size());

    // Start rendering
    commandBuffer->BeginEvent("GBuffer");
    commandBuffer->ImageBarrierBatch({
        { _depthBuffer, TextureLayout::Depth },
        { _normals, TextureLayout::RenderTarget },
        { _albedoEmission, TextureLayout::RenderTarget },
        { _pbrData, TextureLayout::RenderTarget },
        { _emissive, TextureLayout::RenderTarget },
        { _velocityBuffer, TextureLayout::RenderTarget }
    });
    commandBuffer->ClearDepthTarget(_depthBuffer);
    commandBuffer->ClearRenderTarget(_normals, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearRenderTarget(_albedoEmission, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearRenderTarget(_pbrData, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearRenderTarget(_emissive, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearRenderTarget(_velocityBuffer, 0.0f, 0.0f, 0.0f, 1.0f);
    if (_draw) { 
        commandBuffer->SetViewport(0, 0, width, height);
        commandBuffer->SetTopology(Topology::TriangleList);
        commandBuffer->BindRenderTargets({ _normals, _albedoEmission, _pbrData, _emissive, _velocityBuffer }, _depthBuffer);
        commandBuffer->BindGraphicsPipeline(_gbufferPipeline.GraphicsPipeline);

        for (auto model : scene.Models) {
            _totalMeshes += model.Primitives.size();
            for (auto primitive : model.Primitives) {
                // Cull
                if (!scene.Camera.InFrustum(primitive.BoundingBox)) {
                    _culledMeshes++;
                    continue;
                }

                auto material = model.Materials[primitive.MaterialIndex];

                Texture::Ptr albedo = material.HasAlbedo ? material.AlbedoTexture : _whiteTexture;
                Texture::Ptr normal = material.HasNormal ? material.NormalTexture : _whiteTexture;
                Texture::Ptr pbr = material.HasMetallicRoughness ? material.PBRTexture : _blackTexture;
                Texture::Ptr emissive = material.HasEmissive ? material.EmissiveTexture : _blackTexture;
                Texture::Ptr ao = material.HasAO ? material.AOTexture : _whiteTexture;

                struct ModelUpload {
                    glm::mat4 CameraMatrix;
                    glm::mat4 PrevCameraMatrix;
                    glm::mat4 Transform;
                    glm::mat4 PrevTransform;
                };
                ModelUpload matrices = {
                    scene.Camera.Projection() * scene.Camera.View(),
                    scene.PrevViewProj,
                    primitive.Transform,
                    primitive.PrevTransform
                };
                if (_visualizeShadow) {
                    matrices.CameraMatrix = depthProjection * depthView;
                    matrices.PrevCameraMatrix = depthProjection * depthView;
                }

                void *pData;
                primitive.ModelBuffer[frameIndex]->Map(0, 0, &pData);
                memcpy(pData, &matrices, sizeof(matrices));
                primitive.ModelBuffer[frameIndex]->Unmap(0, 0);

                struct Data {
                    uint32_t ModelBuffer;
                    uint32_t AlbedoTexture;
                    uint32_t NormalTexture; 
                    uint32_t PBRTexture;
                    uint32_t EmissiveTexture;
                    uint32_t AOTexture;
                    uint32_t Sampler;
                    float EmissiveStrength;
                    glm::vec2 Jitter;

                    glm::vec2 _Pad0 = glm::vec2(0.0f);
                };
                Data data;
                data.ModelBuffer = primitive.ModelBuffer[frameIndex]->CBV();
                data.AlbedoTexture = albedo->SRV();
                data.NormalTexture = normal->SRV();
                data.PBRTexture = pbr->SRV();
                data.EmissiveTexture = emissive->SRV();
                data.AOTexture = ao->SRV();
                data.Sampler = _anisotropicSampler->BindlesssSampler();
                data.Jitter = _currJitter;
                if (!_jitter) {
                    data.Jitter = glm::vec2(0.0f, 0.0f);
                }
                data.EmissiveStrength = _emissiveStrength;

                commandBuffer->BindVertexBuffer(primitive.VertexBuffer);
                commandBuffer->BindIndexBuffer(primitive.IndexBuffer);
                commandBuffer->PushConstantsGraphics(&data, sizeof(data), 0);
                commandBuffer->DrawIndexed(primitive.IndexCount);
            }
        }
    }
    commandBuffer->ImageBarrierBatch({
        { _normals, TextureLayout::ShaderResource },
        { _albedoEmission, TextureLayout::ShaderResource },
        { _pbrData, TextureLayout::ShaderResource },
        { _emissive, TextureLayout::ShaderResource },
        { _velocityBuffer, TextureLayout::ShaderResource }
    });
    commandBuffer->EndEvent();
}

void Deferred::GBufferPassMesh(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();
    
    OPTICK_GPU_CONTEXT(commandBuffer->GetCommandList());
    OPTICK_GPU_EVENT("Construct GBuffer");

    // Construct matrices
    glm::mat4 depthProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 0.05f, 50.0f);
    glm::mat4 depthView = glm::lookAt(scene.Lights.SunTransform.Position, scene.Lights.SunTransform.Position - scene.Lights.SunTransform.GetFrontVector(), glm::vec3(0.0f, 1.0f, 0.0f));

    // Apply jitter
    _currJitter = _haltonSequence[_jitterCounter];
    _jitterCounter = (_jitterCounter + 1) % (_haltonSequence.size());

    // Start
    commandBuffer->BeginEvent("GBuffer");
    commandBuffer->ImageBarrierBatch({
        { _depthBuffer, TextureLayout::Depth },
        { _normals, TextureLayout::RenderTarget },
        { _albedoEmission, TextureLayout::RenderTarget },
        { _pbrData, TextureLayout::RenderTarget },
        { _emissive, TextureLayout::RenderTarget },
        { _velocityBuffer, TextureLayout::RenderTarget }
    });
    commandBuffer->ClearDepthTarget(_depthBuffer);
    commandBuffer->ClearRenderTarget(_normals, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearRenderTarget(_albedoEmission, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearRenderTarget(_pbrData, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearRenderTarget(_emissive, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearRenderTarget(_velocityBuffer, 0.0f, 0.0f, 0.0f, 1.0f);
    if (_draw) { 
        commandBuffer->SetViewport(0, 0, width, height);
        commandBuffer->SetTopology(Topology::TriangleList);
        commandBuffer->BindRenderTargets({ _normals, _albedoEmission, _pbrData, _emissive, _velocityBuffer }, _depthBuffer);
        commandBuffer->BindMeshPipeline(_gbufferPipelineMesh.MeshPipeline);

        for (auto model : scene.Models) {
            _totalMeshes += model.Primitives.size();
            for (auto primitive : model.Primitives) {
                auto material = model.Materials[primitive.MaterialIndex];

                Texture::Ptr albedo = material.HasAlbedo ? material.AlbedoTexture : _whiteTexture;
                Texture::Ptr normal = material.HasNormal ? material.NormalTexture : _whiteTexture;
                Texture::Ptr pbr = material.HasMetallicRoughness ? material.PBRTexture : _blackTexture;
                Texture::Ptr emissive = material.HasEmissive ? material.EmissiveTexture : _blackTexture;
                Texture::Ptr ao = material.HasAO ? material.AOTexture : _whiteTexture;

                struct ModelUpload {
                    glm::mat4 CameraMatrix;
                    glm::mat4 PrevCameraMatrix;
                    glm::mat4 Transform;
                    glm::mat4 PrevTransform;
                };
                ModelUpload matrices = {
                    scene.Camera.Projection() * scene.Camera.View(),
                    scene.PrevViewProj,
                    primitive.Transform,
                    primitive.PrevTransform
                };
                if (_visualizeShadow) {
                    matrices.CameraMatrix = depthProjection * depthView;
                    matrices.PrevCameraMatrix = depthProjection * depthView;
                }

                void *pData;
                primitive.ModelBuffer[frameIndex]->Map(0, 0, &pData);
                memcpy(pData, &matrices, sizeof(matrices));
                primitive.ModelBuffer[frameIndex]->Unmap(0, 0);

                struct Data {
                    uint32_t Matrices;
                    uint32_t Vertices;
                    uint32_t Indices;
                    uint32_t Meshlets;
                    uint32_t Triangles;
                    uint32_t Albedo;
                    uint32_t Normal;
                    uint32_t PBR;
                    uint32_t Emissive;
                    uint32_t AO;
                    uint32_t Sampler;
                    uint32_t DrawMeshlets;
                    float EmissiveStrenght;
                    glm::vec2 Jitter;
                };
                Data data = {
                    primitive.ModelBuffer[frameIndex]->CBV(),
                    primitive.VertexBuffer->SRV(),
                    primitive.IndexBuffer->SRV(),
                    primitive.MeshletBuffer->SRV(),
                    primitive.MeshletTriangles->SRV(),
                    
                    albedo->SRV(),
                    normal->SRV(),
                    pbr->SRV(),
                    emissive->SRV(),
                    ao->SRV(),
                    _sampler->BindlesssSampler(),

                    _drawMeshlets,
                    _emissiveStrength,
                    _currJitter
                };
                if (!_jitter) {
                    data.Jitter = glm::vec2(0.0f);
                }

                commandBuffer->PushConstantsGraphics(&data, sizeof(data), 0);
                commandBuffer->DispatchMesh(primitive.MeshletCount, 1, 1);
            }
        }
    }
    commandBuffer->ImageBarrierBatch({
        { _normals, TextureLayout::ShaderResource },
        { _albedoEmission, TextureLayout::ShaderResource },
        { _pbrData, TextureLayout::ShaderResource },
        { _emissive, TextureLayout::ShaderResource },
        { _velocityBuffer, TextureLayout::ShaderResource }
    });
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
    _sceneBufferLight[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &data, sizeof(Data));
    _sceneBufferLight[frameIndex]->Unmap(0, 0);

    _lightBuffer[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &scene.Lights.GetGPUData(), sizeof(LightSettings::GPUData));
    _lightBuffer[frameIndex]->Unmap(0, 0);

    glm::ivec4 mode(_mode, _ibl, 0, 0);
    _modeBuffer[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, glm::value_ptr(mode), sizeof(glm::ivec4));
    _modeBuffer[frameIndex]->Unmap(0, 0);

    commandBuffer->BeginEvent("Deferred Lighting");
    commandBuffer->ImageBarrier(_outputImage, TextureLayout::Storage);
    if (_draw) {
        struct Constants {
            uint32_t DepthBuffer;
            uint32_t Normals;
            uint32_t AlbedoEmissive;
            uint32_t PbrAO;
            uint32_t Velocity;
            uint32_t Emissive;
            uint32_t SSAO;

            uint32_t Irradiance;
            uint32_t Prefilter;
            uint32_t BRDF;

            uint32_t ShadowMap;
            uint32_t CubeSampler;
            uint32_t Sampler;
            uint32_t ShadowSampler;
            
            uint32_t SceneBuffer;
            uint32_t LightBuffer;
            uint32_t OutputData;
            uint32_t HDRBuffer;
            
            float direct;
            float indirect;
            float _Pad0;
        };
        Constants constants = {
            _depthBuffer->SRV(),
            _normals->SRV(),
            _albedoEmission->SRV(),
            _pbrData->SRV(),
            _velocityBuffer->SRV(),
            _emissive->SRV(),
            _ssao->SRV(),
            _map.IrradianceMap->SRV(),
            _map.PrefilterMap->SRV(),
            _map.BRDF->SRV(),
            _shadowMap->SRV(),
            _sampler->BindlesssSampler(),
            _cubeSampler->BindlesssSampler(),
            _shadowSampler->BindlesssSampler(),
            _sceneBufferLight[frameIndex]->CBV(),
            _lightBuffer[frameIndex]->CBV(),
            _modeBuffer[frameIndex]->CBV(),
            _outputImage->UAV(),
            _directTerm,
            _indirectTerm,
            0.0f
        };

        commandBuffer->SetViewport(0, 0, width, height);
        commandBuffer->BindComputePipeline(_lightingPipeline.ComputePipeline);
        commandBuffer->PushConstantsCompute(&constants, sizeof(constants), 0);
        commandBuffer->Dispatch(std::ceil(width / 8), std::ceil(height / 8), 1);
    }
    commandBuffer->ImageBarrier(_outputImage, TextureLayout::Storage);
    commandBuffer->EndEvent();
}

void Deferred::Render(Scene& scene, uint32_t width, uint32_t height)
{
    if (_useMesh) {
        GBufferPassClassic(scene, width, height);
    } else {
        GBufferPassMesh(scene, width, height);
    }
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
        if (ImGui::TreeNodeEx("Geometry", ImGuiTreeNodeFlags_Framed)) {
            ImGui::Text("Total Meshes: %d", _totalMeshes);
            ImGui::Text("Culled Meshes: %d", _culledMeshes);

            // TODO: Freeze frustum
            ImGui::Checkbox("Use Mesh Shaders", &_useMesh);
            ImGui::Checkbox("Show Meshlets", &_drawMeshlets);

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Shading", ImGuiTreeNodeFlags_Framed)) {
            ImGui::Checkbox("Visualize Shadows", &_visualizeShadow);

            ImGui::SliderFloat("Direct Light Term", &_directTerm, 0.0f, 2.0f, "%.1f");
            ImGui::SliderFloat("Indirect Light Term", &_indirectTerm, 0.0f, 2.0f, "%.1f");
            ImGui::SliderFloat("Emission Strength", &_emissiveStrength, 0.1f, 10.0f, "%.1f");
        
            static const char* Modes[] = { "Default", "Albedo", "Normal", "Metallic Roughness", "Baked AO", "SSAO", "Emissive", "Direct", "Indirect", "Position", "Velocity" };
            ImGui::Combo("Mode", (int*)&_mode, Modes, 11, 11);

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    _totalMeshes = 0;
    _culledMeshes = 0;
}

void Deferred::Reconstruct()
{
    _gbufferPipeline.CheckForRebuild(_context, "GBuffer Classic");
    _gbufferPipelineMesh.CheckForRebuild(_context, "GBuffer Mesh");
    _lightingPipeline.CheckForRebuild(_context, "Deferred");
}

void Deferred::ConnectEnvironmentMap(EnvironmentMap& map)
{
    _map = map;
}

void Deferred::ConnectShadowMap(Texture::Ptr texture)
{
    _shadowMap = texture;
}
