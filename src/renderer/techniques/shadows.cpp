/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-07-15 00:57:08
 */

#include "shadows.hpp"

#include "ImGui/imgui.h"

struct ShadowParam
{
    glm::mat4 SunMatrix;
};

Shadows::Shadows(RenderContext::Ptr context, ShadowMapResolution resolution)
    : _context(context), _shadowMapResolution(resolution), _shadowPipeline(PipelineType::Graphics)
{
    _shadowPipeline.Specs.Cull = CullMode::Front;
    _shadowPipeline.Specs.Depth = DepthOperation::Less;
    _shadowPipeline.Specs.DepthEnabled = true;
    _shadowPipeline.Specs.DepthClipEnable = false;
    _shadowPipeline.Specs.DepthFormat = TextureFormat::R32Depth;
    _shadowPipeline.Specs.Fill = FillMode::Solid;
    _shadowPipeline.Specs.FormatCount = 0;

    _shadowPipeline.SignatureInfo.Entries = {
        RootSignatureEntry::PushConstants
    };
    _shadowPipeline.SignatureInfo.PushConstantSize = sizeof(glm::mat4) * 2;

    _shadowPipeline.ReflectRootSignature(false);
    _shadowPipeline.AddShaderWatch("shaders/Shadows/ShadowVert.hlsl", "Main", ShaderType::Vertex);
    _shadowPipeline.AddShaderWatch("shaders/Shadows/ShadowFrag.hlsl", "Main", ShaderType::Fragment);
    _shadowPipeline.Build(context);

    _shadowMap = context->CreateTexture(uint32_t(resolution), uint32_t(resolution), TextureFormat::R32Typeless, TextureUsage::DepthTarget, false, "[SHADOW] Shadow Map");
    _shadowMap->BuildDepthTarget(TextureFormat::R32Depth);
    _shadowMap->BuildShaderResource(TextureFormat::R32Float);
}

Shadows::~Shadows()
{

}

void Shadows::Render(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(commandBuffer->GetCommandList());
    OPTICK_GPU_EVENT("Shadow Pass");

    commandBuffer->BeginEvent("Shadow Pass");
    commandBuffer->ClearState();
    commandBuffer->ImageBarrier(_shadowMap, TextureLayout::Depth);
    commandBuffer->ClearDepthTarget(_shadowMap);

    if (_renderShadows) {
        glm::mat4 depthProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 0.05f, 50.0f);
        glm::mat4 depthView = glm::lookAt(scene.Lights.SunTransform.Position, scene.Lights.SunTransform.Position - scene.Lights.SunTransform.GetFrontVector(), glm::vec3(0.0f, 1.0f, 0.0f));

        commandBuffer->SetViewport(0, 0, float(_shadowMapResolution), float(_shadowMapResolution));
        commandBuffer->SetTopology(Topology::TriangleList);
        commandBuffer->BindRenderTargets({}, _shadowMap);
        commandBuffer->BindGraphicsPipeline(_shadowPipeline.GraphicsPipeline);

        for (auto& model : scene.Models) {
            for (auto& primitive : model.Primitives) {
                auto material = model.Materials[primitive.MaterialIndex];

                struct PushConstants {
                    glm::mat4 SunMatrix;
                    glm::mat4 ModelMatrix;
                };
                PushConstants constants = {
                    depthProjection * depthView,
                    primitive.Transform
                };

                commandBuffer->PushConstantsGraphics(&constants, sizeof(constants), 0);
                commandBuffer->BindVertexBuffer(primitive.VertexBuffer);
                commandBuffer->BindIndexBuffer(primitive.IndexBuffer);
                commandBuffer->DrawIndexed(primitive.IndexCount);
            }
        }
    }

    commandBuffer->ImageBarrier(_shadowMap, TextureLayout::ShaderResource);
    commandBuffer->EndEvent();
}

void Shadows::OnUI()
{
    if (ImGui::TreeNodeEx("Shadows", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Render Shadows", &_renderShadows);
        ImGui::Image((ImTextureID)_shadowMap->GetImGuiImage().ptr, ImVec2(256, 256));
        ImGui::TreePop();
    }
}

void Shadows::Reconstruct()
{
    _shadowPipeline.CheckForRebuild(_context, "Shadows");
}
