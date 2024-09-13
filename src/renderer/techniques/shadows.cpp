/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-07-15 00:57:08
 */

#include "shadows.hpp"

#include "ImGui/imgui.h"

struct ShadowParam
{
    glm::mat4 LightMatrix;
    glm::mat4 Model;
};

Shadows::Shadows(RenderContext::Ptr context, ShadowMapResolution resolution)
    : _context(context), _shadowMapResolution(resolution)
{
    ShaderBytecode shadowVert, shadowPix;
    ShaderCompiler::CompileShader("shaders/Shadows/ShadowVertex.hlsl", "Main", ShaderType::Vertex, shadowVert);
    ShaderCompiler::CompileShader("shaders/Shadows/ShadowPixel.hlsl", "Main", ShaderType::Fragment, shadowPix);

    GraphicsPipelineSpecs specs;
    specs.Bytecodes[ShaderType::Vertex] = shadowVert;
    specs.Bytecodes[ShaderType::Fragment] = shadowPix;
    specs.Cull = CullMode::Back;
    specs.Depth = DepthOperation::Less;
    specs.DepthEnabled = true;
    specs.DepthFormat = TextureFormat::R32Depth;
    specs.Fill = FillMode::Solid;
    specs.FormatCount = 0;

    _shadowPipeline = context->CreateGraphicsPipeline(specs);

    _shadowMap = context->CreateTexture(uint32_t(resolution), uint32_t(resolution), TextureFormat::R32Typeless, TextureUsage::DepthTarget, false, "Shadow Map");
    _shadowMap->BuildDepthTarget(TextureFormat::R32Depth);
    _shadowMap->BuildShaderResource(TextureFormat::R32Float);
    
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        _shadowParam[i] = context->CreateBuffer(256, 0, BufferType::Constant, false, "Shadow Param (" + std::to_string(i) + ")");
        _shadowParam[i]->BuildConstantBuffer();
    }
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

    float near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 depthProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 depthView = glm::lookAt(scene.Lights.SunPosition, glm::vec3(-scene.Lights.Sun.Direction), glm::vec3(0.0f, 1.0f, 0.0f)); // Kill yourself

    ShadowParam param;
    param.LightMatrix = depthProjection * depthView;
    param.Model = glm::mat4(1.0f);

    void *pData;
    _shadowParam[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &param, sizeof(ShadowParam));
    _shadowParam[frameIndex]->Unmap(0, 0);

    commandBuffer->BeginEvent("Shadow Pass");
    commandBuffer->ImageBarrier(_shadowMap, TextureLayout::Depth);
    commandBuffer->SetViewport(0, 0, float(_shadowMapResolution), float(_shadowMapResolution));
    commandBuffer->SetTopology(Topology::TriangleList);
    commandBuffer->BindRenderTargets({}, _shadowMap);
    commandBuffer->ClearDepthTarget(_shadowMap);
    commandBuffer->BindGraphicsPipeline(_shadowPipeline);
    commandBuffer->BindGraphicsConstantBuffer(_shadowParam[frameIndex], 0);

    for (auto& model : scene.Models) {
        for (auto& primitive : model.Primitives) {
            commandBuffer->BindVertexBuffer(primitive.VertexBuffer);
            commandBuffer->BindIndexBuffer(primitive.IndexBuffer);
            commandBuffer->DrawIndexed(primitive.IndexCount);
        }
    }

    commandBuffer->ImageBarrier(_shadowMap, TextureLayout::ShaderResource);
    commandBuffer->EndEvent();
}

void Shadows::OnUI()
{
    if (ImGui::TreeNodeEx("Shadows", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Image((ImTextureID)_shadowMap->GetImGuiImage().ptr, ImVec2(256, 256));
        ImGui::TreePop();
    }
}