/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 17:45:00
 */

#include "forward.hpp"

#include <ImGui/imgui.h>
#include <optick.h>

Forward::Forward(RenderContext::Ptr context)
    : _context(context)
{
    uint32_t width, height;
    context->GetWindow()->GetSize(width, height);

    _whiteTexture = context->CreateTexture(1, 1, TextureFormat::RGBA8, TextureUsage::ShaderResource, false, "White Texture");
    _whiteTexture->BuildShaderResource();

    _blackTexture = context->CreateTexture(1, 1, TextureFormat::RGBA8, TextureUsage::ShaderResource, false, "Black Texture");
    _blackTexture->BuildShaderResource();

    Uploader uploader = context->CreateUploader();

    {
        uint32_t color = 0xFFFFFFFF;
        Bitmap image;
        image.Width = 1;
        image.Height = 1;
        image.Delete = false;
        image.Bytes = reinterpret_cast<char*>(&color);
        uploader.CopyHostToDeviceTexture(image, _whiteTexture);
    }
    {
        uint32_t color = 0xFF000000;
        Bitmap image;
        image.Width = 1;
        image.Height = 1;
        image.Delete = false;
        image.Bytes = reinterpret_cast<char*>(&color);
        uploader.CopyHostToDeviceTexture(image, _blackTexture);
    }

    _context->FlushUploader(uploader);

    _outputImage = context->CreateTexture(width, height, TextureFormat::RGBA16Unorm, TextureUsage::RenderTarget, false, "Forward RTV");
    _outputImage->BuildRenderTarget();
    _outputImage->BuildShaderResource();

    _depthBuffer = context->CreateTexture(width, height, TextureFormat::R32Depth, TextureUsage::DepthTarget, false, "Forward DSV");
    _depthBuffer->BuildDepthTarget();

    {
        GraphicsPipelineSpecs specs;
        specs.FormatCount = 1;
        specs.Formats[0] = TextureFormat::RGBA16Unorm;
        specs.DepthFormat = TextureFormat::R32Depth;
        specs.Depth = DepthOperation::Less;
        specs.DepthEnabled = true;
        specs.Cull = CullMode::Front;
        specs.Fill = FillMode::Solid;
        ShaderCompiler::CompileShader("shaders/Forward/PBR/PBRVert.hlsl", "Main", ShaderType::Vertex, specs.Bytecodes[ShaderType::Vertex]);
        ShaderCompiler::CompileShader("shaders/Forward/PBR/PBRFrag.hlsl", "Main", ShaderType::Fragment, specs.Bytecodes[ShaderType::Fragment]);

        _pbrPipeline = context->CreateGraphicsPipeline(specs);
    }

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        _sceneBuffer[i] = context->CreateBuffer(256, 0, BufferType::Constant, false, "Scene Buffer CBV");
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
    if (ImGui::TreeNodeEx("Forward", ImGuiTreeNodeFlags_Framed))
    {
        ImGui::Checkbox("Enable IBL", &_ibl);

        static const char* Modes[] = { "Default", "Albedo", "Normal", "Metallic Roughness", "AO", "Emissive", "Specular", "Ambient" };
        ImGui::Combo("Mode", (int*)&_mode, Modes, 8);

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

    // TODO(amelie): calculate view matrix

    float near_plane = 1.0f, far_plane = 10.0f;
    glm::mat4 depthProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 depthView = glm::translate(glm::mat4(1.0f), scene.Lights.SunPosition)
                                     * glm::rotate(glm::mat4(1.0f), glm::radians(scene.Lights.Sun.Direction.x), glm::vec3(1.0f, 0.0f, 0.0f))
                                     * glm::rotate(glm::mat4(1.0f), glm::radians(scene.Lights.Sun.Direction.y), glm::vec3(0.0f, 1.0f, 0.0f))
                                     * glm::rotate(glm::mat4(1.0f), glm::radians(scene.Lights.Sun.Direction.z), glm::vec3(0.0f, 0.0f, 1.0f));

    struct Data {
        glm::mat4 View;
        glm::mat4 Projection;
        glm::vec4 CameraPosition;
        glm::mat4 SunMatrix;
    };
    Data data;
    data.View = scene.Camera.View();
    data.Projection = scene.Camera.Projection();
    data.CameraPosition = glm::vec4(scene.Camera.GetPosition(), 1.0f);
    data.SunMatrix = depthView * depthProjection;

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
    commandBuffer->SetViewport(0, 0, width, height);
    commandBuffer->SetTopology(Topology::TriangleList);
    commandBuffer->BindRenderTargets({ _outputImage }, _depthBuffer);
    commandBuffer->ClearRenderTarget(_outputImage, 0.3f, 0.5f, 0.8f, 1.0f);
    commandBuffer->ClearDepthTarget(_depthBuffer);
    commandBuffer->BindGraphicsPipeline(_pbrPipeline);
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

    commandBuffer->EndEvent();
}
