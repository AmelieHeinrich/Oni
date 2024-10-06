//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-15 00:11:47
//

#include "debug_renderer.hpp"

#include <imgui/imgui.h>

static std::shared_ptr<DebugRenderer> Renderer;

void DebugRenderer::SetDebugRenderer(std::shared_ptr<DebugRenderer> renderer)
{
    Renderer = renderer;
}

std::shared_ptr<DebugRenderer> DebugRenderer::Get()
{
    return Renderer;
}

DebugRenderer::DebugRenderer(RenderContext::Ptr context, Texture::Ptr output)
    : LineShader(PipelineType::Graphics), _context(context), MotionShader(PipelineType::Compute), AABBShader(PipelineType::Graphics)
{
    Context = context;
    Output = output;

    // Line shader
    LineShader.Specs.Fill = FillMode::Solid;
    LineShader.Specs.Cull = CullMode::None;
    LineShader.Specs.DepthEnabled = false;
    LineShader.Specs.Depth = DepthOperation::None;
    LineShader.Specs.DepthFormat = TextureFormat::None;
    LineShader.Specs.Formats[0] = TextureFormat::RGBA8;
    LineShader.Specs.FormatCount = 1;
    LineShader.Specs.Line = true;

    LineShader.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(glm::mat4)
    };
    LineShader.ReflectRootSignature(false);
    LineShader.AddShaderWatch("shaders/DebugRenderer/LineRendererVert.hlsl", "Main", ShaderType::Vertex);
    LineShader.AddShaderWatch("shaders/DebugRenderer/LineRendererFrag.hlsl", "Main", ShaderType::Fragment);
    LineShader.Build(context);

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        LineTransferBuffer[i] = context->CreateBuffer(MAX_LINES * sizeof(LineVertex), 0, BufferType::Constant, false, "[DEBUG] Line Transfer Buffer");
        LineVertexBuffer[i] = context->CreateBuffer(MAX_LINES * sizeof(LineVertex), sizeof(LineVertex), BufferType::Vertex, false, "[DEBUG] Line Vertex Buffer");
    }

    // AABB shader
    AABBShader.Specs.Fill = FillMode::Line;
    AABBShader.Specs.Cull = CullMode::None;
    AABBShader.Specs.DepthEnabled = false;
    AABBShader.Specs.Depth = DepthOperation::None;
    AABBShader.Specs.DepthFormat = TextureFormat::None;
    AABBShader.Specs.Formats[0] = TextureFormat::RGBA8;
    AABBShader.Specs.FormatCount = 1;

    AABBShader.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(glm::mat4)
    };
    AABBShader.ReflectRootSignature(false);
    AABBShader.AddShaderWatch("shaders/DebugRenderer/AABBRendererVert.hlsl", "Main", ShaderType::Vertex);
    AABBShader.AddShaderWatch("shaders/DebugRenderer/AABBRendererFrag.hlsl", "Main", ShaderType::Fragment);
    AABBShader.Build(context);

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        AABBTransferBuffer[i] = context->CreateBuffer(MAX_LINES * sizeof(CubeVertex), 0, BufferType::Constant, false, "[DEBUG] AABB Transfer Buffer");
        AABBVertexBuffer[i] = context->CreateBuffer(MAX_LINES * sizeof(CubeVertex), sizeof(CubeVertex), BufferType::Vertex, false, "[DEBUG] AABB Vertex Buffer");
    }

    // Motion viz
    MotionShader.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        sizeof(uint32_t) * 3
    };
    MotionShader.ReflectRootSignature(false);
    MotionShader.AddShaderWatch("shaders/DebugRenderer/MotionVisualizerCompute.hlsl", "Main", ShaderType::Compute);
    MotionShader.Build(context);

    NearestSampler = context->CreateSampler(SamplerAddress::Border, SamplerFilter::Nearest, false, 0);
}

void DebugRenderer::Resize(uint32_t width, uint32_t height, Texture::Ptr output)
{
    Output = output;
}

void DebugRenderer::OnUI()
{
    if (ImGui::TreeNodeEx("Debug Renderer", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Draw Lines", &DrawLines);
        ImGui::Checkbox("Draw AABB", &DrawAABB);
        ImGui::Checkbox("Visualize Motion Vectors", &DrawMotion);

        ImGui::Separator();
        ImGui::Text("Line Count: %d", List.Lines.size());
        ImGui::Text("AABB Count: %d", List.BoundingBoxes.size());
        ImGui::TreePop();
    }
}

void DebugRenderer::PushLine(glm::vec3 a, glm::vec3 b, glm::vec3 color)
{
    List.Lines.push_back(Line {
        a,
        b,
        color
    });
}

void DebugRenderer::PushAABB(AABB aabb, glm::mat4 transform)
{
    List.BoundingBoxes.push_back({ aabb, transform });
}

void DebugRenderer::Flush(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr cmdBuffer = Context->GetCurrentCommandBuffer();
    uint32_t frameIndex = Context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(cmdBuffer->GetCommandList());
    OPTICK_GPU_EVENT("Debug Renderer");

    if (DrawAABB) {
        for (auto& model : scene.Models) {
            for (auto& primitive : model.Primitives) {
                PushAABB(primitive.BoundingBox, primitive.Transform);
            }
        }
    }

    cmdBuffer->BeginEvent("Debug Renderer");
    cmdBuffer->ImageBarrier(Output, TextureLayout::RenderTarget);

    if (DrawLines) {
        if (!List.Lines.empty()) {
            std::vector<LineVertex> vertices;
            for (auto& line : List.Lines) {
                vertices.push_back(LineVertex {
                    glm::vec4(line.A, 1.0f),
                    glm::vec4(line.Color, 1.0f)
                });
                vertices.push_back(LineVertex {
                    glm::vec4(line.B, 1.0f),
                    glm::vec4(line.Color, 1.0f)
                });
            }

            glm::mat4 mvp = scene.Camera.Projection() * scene.Camera.View();

            void *pData;
            LineTransferBuffer[frameIndex]->Map(0, 0, &pData);
            memcpy(pData, vertices.data(), sizeof(LineVertex) * vertices.size());
            LineTransferBuffer[frameIndex]->Unmap(0, 0);

            cmdBuffer->BeginEvent("Lines", 200, 200, 200);
            cmdBuffer->CopyBufferToBuffer(LineVertexBuffer[frameIndex], LineTransferBuffer[frameIndex]);
            cmdBuffer->SetViewport(0, 0, width, height);
            cmdBuffer->SetTopology(Topology::LineList);
            cmdBuffer->BindRenderTargets({ Output }, nullptr);
            cmdBuffer->BindGraphicsPipeline(LineShader.GraphicsPipeline);
            cmdBuffer->PushConstantsGraphics(glm::value_ptr(mvp), sizeof(glm::mat4), 0);
            cmdBuffer->BindVertexBuffer(LineVertexBuffer[frameIndex]);
            cmdBuffer->Draw(vertices.size());
            cmdBuffer->EndEvent();
        }
    }
    if (DrawAABB) {
        // Generate points of cube
        std::vector<glm::vec4> vertices;
        for (int i = 0; i < List.BoundingBoxes.size(); i++) {
            AABB boundingBox = List.BoundingBoxes[i].BoundingBox;
            glm::mat4 transform = List.BoundingBoxes[i].Transform;

            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x - boundingBox.Extent.x, boundingBox.Center.y - boundingBox.Extent.y, boundingBox.Center.z - boundingBox.Extent.z), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x,                        boundingBox.Center.y,                        boundingBox.Center.z                       ), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x - boundingBox.Extent.x, boundingBox.Center.y - boundingBox.Extent.y, boundingBox.Center.z                       ), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x,                        boundingBox.Center.y,                        boundingBox.Center.z + boundingBox.Extent.z), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x - boundingBox.Extent.x, boundingBox.Center.y,                        boundingBox.Center.z - boundingBox.Extent.z), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x,                        boundingBox.Center.y + boundingBox.Extent.y, boundingBox.Center.z                       ), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x - boundingBox.Extent.x, boundingBox.Center.y,                        boundingBox.Center.z                       ), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x,                        boundingBox.Center.y + boundingBox.Extent.y, boundingBox.Center.z + boundingBox.Extent.z), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x,                        boundingBox.Center.y - boundingBox.Extent.y, boundingBox.Center.z - boundingBox.Extent.z), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x + boundingBox.Extent.x, boundingBox.Center.y,                        boundingBox.Center.z                       ), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x,                        boundingBox.Center.y - boundingBox.Extent.y, boundingBox.Center.z                       ), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x + boundingBox.Extent.x, boundingBox.Center.y,                        boundingBox.Center.z + boundingBox.Extent.z), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x,                        boundingBox.Center.y,                        boundingBox.Center.z - boundingBox.Extent.z), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x + boundingBox.Extent.x, boundingBox.Center.y + boundingBox.Extent.y, boundingBox.Center.z                       ), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x,                        boundingBox.Center.y,                        boundingBox.Center.z                       ), transform));
            vertices.push_back(ApplyTransform(glm::vec3(boundingBox.Center.x + boundingBox.Extent.x, boundingBox.Center.y + boundingBox.Extent.y, boundingBox.Center.z + boundingBox.Extent.z), transform));
        }

        glm::mat4 mvp = scene.Camera.Projection() * scene.Camera.View();

        void *pData;
        AABBTransferBuffer[frameIndex]->Map(0, 0, &pData);
        memcpy(pData, vertices.data(), sizeof(CubeVertex) * vertices.size());
        AABBTransferBuffer[frameIndex]->Unmap(0, 0);

        cmdBuffer->BeginEvent("AABB", 200, 200, 200);
        cmdBuffer->CopyBufferToBuffer(AABBVertexBuffer[frameIndex], AABBTransferBuffer[frameIndex]);
        cmdBuffer->SetViewport(0, 0, width, height);
        cmdBuffer->SetTopology(Topology::TriangleStrip);
        cmdBuffer->BindRenderTargets({ Output }, nullptr);
        cmdBuffer->BindGraphicsPipeline(AABBShader.GraphicsPipeline);
        cmdBuffer->PushConstantsGraphics(glm::value_ptr(mvp), sizeof(glm::mat4), 0);
        cmdBuffer->BindVertexBuffer(AABBVertexBuffer[frameIndex]);
        cmdBuffer->Draw(vertices.size());
        cmdBuffer->EndEvent();
    }
    if (DrawMotion) {
        if (VelocityBuffer) {
            struct Constants {
                uint32_t Velocity;
                uint32_t Output;
                uint32_t LinearSampler;
            };
            Constants constants = {
                VelocityBuffer->SRV(),
                Output->UAV(),
                NearestSampler->BindlesssSampler()
            };

            cmdBuffer->BeginEvent("Motion Visualizer", 200, 200, 200);
            cmdBuffer->ImageBarrierBatch({
                { VelocityBuffer, TextureLayout::ShaderResource },
                { Output, TextureLayout::Storage }
            });
            cmdBuffer->BindComputePipeline(MotionShader.ComputePipeline);
            cmdBuffer->PushConstantsCompute(&constants, sizeof(constants), 0);
            cmdBuffer->Dispatch(std::ceil(width / 8), std::ceil(height / 8), 1);
            cmdBuffer->EndEvent();
        }
    }

    cmdBuffer->ImageBarrier(Output, TextureLayout::ShaderResource);
    cmdBuffer->EndEvent();
}

Texture::Ptr DebugRenderer::GetOutput()
{
    return Output;
}

void DebugRenderer::Reset()
{
    List.Lines.clear();
    List.BoundingBoxes.clear();
}

void DebugRenderer::SetVelocityBuffer(Texture::Ptr texture)
{
    VelocityBuffer = texture;
}

void DebugRenderer::Reconstruct()
{
    LineShader.CheckForRebuild(_context, "Line Shader");
    MotionShader.CheckForRebuild(_context, "Motion Visualization");
    AABBShader.CheckForRebuild(_context, "AABB Shader");
}
