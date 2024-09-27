/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 17:45:00
 */

#include "forward.hpp"

#include <ImGui/imgui.h>
#include <optick.h>

Forward::Forward(RenderContext::Ptr context)
    : _context(context), _pbrPipeline(PipelineType::Graphics)
{
    uint32_t width, height;
    context->GetWindow()->GetSize(width, height);

    _whiteTexture = context->CreateTexture(1, 1, TextureFormat::RGBA8, TextureUsage::ShaderResource, false, "White Texture");
    _whiteTexture->BuildShaderResource();

    Uploader uploader = context->CreateUploader();

    uint32_t color = 0xFFFFFFFF;
    Bitmap image;
    image.Width = 1;
    image.Height = 1;
    image.Delete = false;
    image.Bytes = reinterpret_cast<char*>(&color);
    uploader.CopyHostToDeviceTexture(image, _whiteTexture);

    _context->FlushUploader(uploader);

    _outputImage = context->CreateTexture(width, height, TextureFormat::RGBA16Unorm, TextureUsage::RenderTarget, false, "Forward RTV");
    _outputImage->BuildRenderTarget();
    _outputImage->BuildShaderResource();

    _depthBuffer = context->CreateTexture(width, height, TextureFormat::R32Depth, TextureUsage::DepthTarget, false, "Forward DSV");
    _depthBuffer->BuildDepthTarget();

    {
        _pbrPipeline.Specs.FormatCount = 1;
        _pbrPipeline.Specs.Formats[0] = TextureFormat::RGBA16Unorm;
        _pbrPipeline.Specs.DepthFormat = TextureFormat::R32Depth;
        _pbrPipeline.Specs.Depth = DepthOperation::Less;
        _pbrPipeline.Specs.DepthEnabled = true;
        _pbrPipeline.Specs.Cull = CullMode::Front;
        _pbrPipeline.Specs.Fill = FillMode::Solid;

        _pbrPipeline.AddShaderWatch("shaders/Forward/PBR/PBRVert.hlsl", "Main", ShaderType::Vertex);
        _pbrPipeline.AddShaderWatch("shaders/Forward/PBR/PBRFrag.hlsl", "Main", ShaderType::Fragment);
        _pbrPipeline.Build(context);
    }

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        _sceneBuffer[i] = context->CreateBuffer(512, 0, BufferType::Constant, false, "Scene Buffer CBV");
        _sceneBuffer[i]->BuildConstantBuffer();
    
        _lightBuffer[i] = context->CreateBuffer(24832, 0, BufferType::Constant, false, "Light Buffer CBV");
        _lightBuffer[i]->BuildConstantBuffer();

        _modeBuffer[i] = context->CreateBuffer(256, 0, BufferType::Constant, false, "Mode Buffer CBV");
        _modeBuffer[i]->BuildConstantBuffer();
    }

    _sampler = context->CreateSampler(SamplerAddress::Wrap, SamplerFilter::Linear, true, 0);
    _shadowSampler = context->CreateSampler(SamplerAddress::Clamp, SamplerFilter::Linear, false, 0);
}

Forward::~Forward()
{

}

void Forward::Render(Scene& scene, uint32_t width, uint32_t height)
{
    RenderPBR(scene, width, height);
}

void Forward::Resize(uint32_t width, uint32_t height)
{
    _outputImage.reset();
    _depthBuffer.reset();

    _outputImage = _context->CreateTexture(width, height, TextureFormat::RGBA16Unorm, TextureUsage::RenderTarget, false, "Forward RTV");
    _outputImage->BuildRenderTarget();
    _outputImage->BuildShaderResource();

    _depthBuffer = _context->CreateTexture(width, height, TextureFormat::R32Depth, TextureUsage::DepthTarget, false, "Forward DSV");
    _depthBuffer->BuildDepthTarget();
}

void Forward::OnUI()
{
    if (ImGui::TreeNodeEx("Forward", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Draw Geometry", &_draw);
        ImGui::Checkbox("Enable IBL", &_ibl);
        ImGui::Checkbox("Visualize Shadows", &_visualizeShadow);

        static const char* Modes[] = { "Default", "Albedo", "Normal", "Metallic Roughness", "AO", "Emissive", "Specular", "Ambient", "Position" };
        ImGui::Combo("Mode", (int*)&_mode, Modes, 9);

        ImGui::TreePop();
    }
}

void Forward::ConnectEnvironmentMap(EnvironmentMap& map)
{
    _map = map;
}

void Forward::ConnectShadowMap(Texture::Ptr texture)
{
    _shadowMap = texture;
}

void Forward::RenderPBR(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(commandBuffer->GetCommandList());
    OPTICK_GPU_EVENT("PBR Forward");

    glm::mat4 depthProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 0.05f, 50.0f);
    glm::mat4 depthView = glm::lookAt(scene.Lights.SunTransform.Position, scene.Lights.SunTransform.Position - scene.Lights.SunTransform.GetFrontVector(), glm::vec3(0.0f, 1.0f, 0.0f));

    struct Data {
        glm::mat4 CameraMatrix;
        glm::mat4 SunMatrix;
        glm::vec4 CameraPosition;
        glm::vec3 _Pad0;
    };
    Data data;
    if (!_visualizeShadow) {
        data.CameraMatrix = scene.Camera.Projection() * scene.Camera.View();
    } else {
        data.CameraMatrix = depthProjection * depthView;
    }
    data.CameraPosition = glm::vec4(scene.Camera.GetPosition(), 1.0f);
    data.SunMatrix = depthProjection * depthView;

    void *pData;
    _sceneBuffer[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &data, sizeof(Data));
    _sceneBuffer[frameIndex]->Unmap(0, 0);

    _lightBuffer[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &scene.Lights.GetGPUData(), sizeof(LightSettings::GPUData));
    _lightBuffer[frameIndex]->Unmap(0, 0);

    glm::ivec4 mode(_mode, _ibl, 0, 0);
    _modeBuffer[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, glm::value_ptr(mode), sizeof(glm::ivec4));
    _modeBuffer[frameIndex]->Unmap(0, 0);

    commandBuffer->BeginEvent("Forward Pass");
    commandBuffer->ImageBarrier(_outputImage, TextureLayout::RenderTarget);
    commandBuffer->ClearRenderTarget(_outputImage, 0.0f, 0.0f, 0.0f, 1.0f);
    commandBuffer->ClearDepthTarget(_depthBuffer);
    if (_draw) {
        commandBuffer->SetViewport(0, 0, width, height);
        commandBuffer->SetTopology(Topology::TriangleList);
        commandBuffer->BindRenderTargets({ _outputImage }, _depthBuffer);
        commandBuffer->BindGraphicsPipeline(_pbrPipeline.GraphicsPipeline);
        commandBuffer->BindGraphicsConstantBuffer(_sceneBuffer[frameIndex], 0);
        commandBuffer->BindGraphicsCubeMap(_map.IrradianceMap, 7);
        commandBuffer->BindGraphicsCubeMap(_map.PrefilterMap, 8);
        commandBuffer->BindGraphicsShaderResource(_map.BRDF, 9);
        commandBuffer->BindGraphicsShaderResource(_shadowMap, 10);
        commandBuffer->BindGraphicsSampler(_sampler, 11);
        commandBuffer->BindGraphicsSampler(_shadowSampler, 12);
        commandBuffer->BindGraphicsConstantBuffer(_lightBuffer[frameIndex], 13);
        commandBuffer->BindGraphicsConstantBuffer(_modeBuffer[frameIndex], 14);

        for (auto model : scene.Models) {
            for (auto primitive : model.Primitives) {
                //if (!scene.Camera.InFrustum(primitive.BoundingBox)) {
                //    continue;
                //}

                auto material = model.Materials[primitive.MaterialIndex];

                Texture::Ptr albedo = material.HasAlbedo ? material.AlbedoTexture : _whiteTexture;
                Texture::Ptr normal = material.HasNormal ? material.NormalTexture : _whiteTexture;
                Texture::Ptr pbr = material.HasMetallicRoughness ? material.PBRTexture : _whiteTexture;
                Texture::Ptr emissive = material.HasEmissive ? material.EmissiveTexture : _whiteTexture;
                Texture::Ptr ao = material.HasAO ? material.AOTexture : _whiteTexture;

                struct ModelData {
                    glm::mat4 Transform;
                    glm::vec4 FlatColor;
                };
                ModelData modelData = {
                    primitive.Transform,
                    glm::vec4(material.FlatColor, 1.0f)
                };

                primitive.ModelBuffer[frameIndex]->Map(0, 0, &pData);
                memcpy(pData, &modelData, sizeof(ModelData));
                primitive.ModelBuffer[frameIndex]->Unmap(0, 0);

                commandBuffer->BindVertexBuffer(primitive.VertexBuffer);
                commandBuffer->BindIndexBuffer(primitive.IndexBuffer);
                commandBuffer->BindGraphicsConstantBuffer(primitive.ModelBuffer[frameIndex], 1);
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

void Forward::Reconstruct()
{
    _pbrPipeline.CheckForRebuild(_context, "Forward");
}
