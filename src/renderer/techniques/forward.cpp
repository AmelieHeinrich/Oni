/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 17:45:00
 */

#include "forward.hpp"

#include <ImGui/imgui.h>

Forward::Forward(RenderContext::Ptr context)
    : _context(context)
{
    uint32_t width, height;
    context->GetWindow()->GetSize(width, height);

    _whiteTexture = context->CreateTexture(1, 1, TextureFormat::RGBA8, TextureUsage::ShaderResource, false, "White Texture");
    _whiteTexture->BuildShaderResource();

    uint32_t color = 0xFFFFFFFF;
    Image image;
    image.Width = 1;
    image.Height = 1;
    image.Delete = false;
    image.Bytes = reinterpret_cast<char*>(&color);

    Uploader uploader = context->CreateUploader();
    uploader.CopyHostToDeviceTexture(image, _whiteTexture);
    _context->FlushUploader(uploader);

    _outputImage = context->CreateTexture(width, height, TextureFormat::RGBA16Unorm, TextureUsage::RenderTarget, false, "Forward RTV");
    _outputImage->BuildRenderTarget();
    _outputImage->BuildShaderResource();

    _depthBuffer = context->CreateTexture(width, height, TextureFormat::R32Depth, TextureUsage::DepthTarget, false, "Forward DSV");
    _depthBuffer->BuildDepthTarget();

    GraphicsPipelineSpecs specs;
    specs.FormatCount = 1;
    specs.Formats[0] = TextureFormat::RGBA16Unorm;
    specs.DepthFormat = TextureFormat::R32Depth;
    specs.Depth = DepthOperation::Less;
    specs.DepthEnabled = true;
    specs.Cull = CullMode::None;
    specs.Fill = FillMode::Solid;
    ShaderCompiler::CompileShader("shaders/Forward/ForwardVert.hlsl", "Main", ShaderType::Vertex, specs.Bytecodes[ShaderType::Vertex]);
    ShaderCompiler::CompileShader("shaders/Forward/ForwardFrag.hlsl", "Main", ShaderType::Fragment, specs.Bytecodes[ShaderType::Fragment]);

    _forwardPipeline = context->CreateGraphicsPipeline(specs);

    _sceneBuffer = context->CreateBuffer(256, 0, BufferType::Constant, false, "Scene Buffer CBV");
    _sceneBuffer->BuildConstantBuffer();
    
    _modelBuffer = context->CreateBuffer(256, 0, BufferType::Constant, false, "Model Buffer CBV");
    _modelBuffer->BuildConstantBuffer();

    _lightBuffer = context->CreateBuffer(16640, 0, BufferType::Constant, false, "Light Buffer CBV");
    _lightBuffer->BuildConstantBuffer();

    _modeBuffer = context->CreateBuffer(256, 0, BufferType::Constant, false, "Mode Buffer CBV");
    _modeBuffer->BuildConstantBuffer();

    _sampler = context->CreateSampler(SamplerAddress::Wrap, SamplerFilter::Linear, true, 0);
}

Forward::~Forward()
{

}

void Forward::Render(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();

    struct Data {
        glm::mat4 View;
        glm::mat4 Projection;
        glm::vec4 CameraPosition;
    };
    Data data;
    data.View = scene.View;
    data.Projection = scene.Projection;
    data.CameraPosition = scene.CameraPosition;

    void *pData;
    _sceneBuffer->Map(0, 0, &pData);
    memcpy(pData, &data, sizeof(Data));
    _sceneBuffer->Unmap(0, 0);

    _lightBuffer->Map(0, 0, &pData);
    memcpy(pData, &scene.LightBuffer, sizeof(LightData));
    _lightBuffer->Unmap(0, 0);

    glm::ivec4 mode(_mode, 0, 0, 0);
    _modeBuffer->Map(0, 0, &pData);
    memcpy(pData, glm::value_ptr(mode), sizeof(glm::ivec4));
    _modeBuffer->Unmap(0, 0);

    commandBuffer->Begin();
    commandBuffer->ImageBarrier(_outputImage, TextureLayout::RenderTarget);
    commandBuffer->SetViewport(0, 0, width, height);
    commandBuffer->SetTopology(Topology::TriangleList);
    commandBuffer->BindRenderTargets({ _outputImage }, _depthBuffer);
    commandBuffer->ClearRenderTarget(_outputImage, 0.3f, 0.5f, 0.8f, 1.0f);
    commandBuffer->ClearDepthTarget(_depthBuffer);
    commandBuffer->BindGraphicsPipeline(_forwardPipeline);
    commandBuffer->BindGraphicsConstantBuffer(_sceneBuffer, 0);
    commandBuffer->BindGraphicsCubeMap(_map.IrradianceMap, 7);
    commandBuffer->BindGraphicsCubeMap(_map.PrefilterMap, 8);
    commandBuffer->BindGraphicsShaderResource(_map.BRDF, 9);
    commandBuffer->BindGraphicsSampler(_sampler, 10);
    commandBuffer->BindGraphicsConstantBuffer(_lightBuffer, 11);
    commandBuffer->BindGraphicsConstantBuffer(_modeBuffer, 12);

    for (auto& model : scene.Models) {
        for (auto& primitive : model.Primitives) {
            auto& material = model.Materials[primitive.MaterialIndex];

            Texture::Ptr albedo = material.HasAlbedo ? material.AlbedoTexture : _whiteTexture;
            Texture::Ptr normal = material.HasNormal ? material.NormalTexture : _whiteTexture;
            Texture::Ptr pbr = material.HasMetallicRoughness ? material.PBRTexture : _whiteTexture;
            Texture::Ptr emissive = material.HasEmissive ? material.EmissiveTexture : _whiteTexture;
            Texture::Ptr ao = material.HasAO ? material.AOTexture : _whiteTexture;

            struct ModelData {
                glm::mat4 Transform;
                glm::vec4 FlatColor;
            };
            ModelData temp;
            temp.Transform = primitive.Transform;
            temp.FlatColor = glm::vec4(material.FlatColor, 1.0f);

            _modelBuffer->Map(0, 0, &pData);
            memcpy(pData, &temp, sizeof(ModelData));
            _modelBuffer->Unmap(0, 0);

            commandBuffer->BindVertexBuffer(primitive.VertexBuffer);
            commandBuffer->BindIndexBuffer(primitive.IndexBuffer);
            commandBuffer->BindGraphicsConstantBuffer(_modelBuffer, 1);
            commandBuffer->BindGraphicsShaderResource(albedo, 2);
            commandBuffer->BindGraphicsShaderResource(normal, 3);
            commandBuffer->BindGraphicsShaderResource(pbr, 4);
            commandBuffer->BindGraphicsShaderResource(emissive, 5);
            commandBuffer->BindGraphicsShaderResource(ao, 6);
            commandBuffer->DrawIndexed(primitive.IndexCount);
        }
    }

    commandBuffer->End();
    _context->ExecuteCommandBuffers({ commandBuffer }, CommandQueueType::Graphics);
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
        static const char* Modes[] = { "Default", "Albedo", "Normal", "Metallic Roughness", "AO", "Emissive", "Specular", "Ambient" };
        ImGui::Combo("Mode", (int*)&_mode, Modes, 8);

        ImGui::TreePop();
    }
}

void Forward::ConnectEnvironmentMap(EnvironmentMap& map)
{
    _map = map;
}
