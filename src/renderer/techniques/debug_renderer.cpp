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
    : LineShader(PipelineType::Graphics), _context(context), MotionShader(PipelineType::Compute)
{
    Context = context;
    Output = output;

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
        ImGui::Checkbox("Visualize Motion Vectors", &DrawMotion);
        ImGui::Text("Line Count: %d", List.Lines.size());
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

void DebugRenderer::Flush(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr cmdBuffer = Context->GetCurrentCommandBuffer();
    uint32_t frameIndex = Context->GetBackBufferIndex();

    OPTICK_GPU_CONTEXT(cmdBuffer->GetCommandList());
    OPTICK_GPU_EVENT("Debug Renderer");

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

            Uploader uploader = Context->CreateUploader();
            uploader.CopyBufferToBuffer(LineTransferBuffer[frameIndex], LineVertexBuffer[frameIndex]);
            Context->FlushUploader(uploader, cmdBuffer);

            cmdBuffer->BeginEvent("Lines");
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

            cmdBuffer->BeginEvent("Motion Visualizer");
            cmdBuffer->ImageBarrier(VelocityBuffer, TextureLayout::ShaderResource);
            cmdBuffer->ImageBarrier(Output, TextureLayout::Storage);
            cmdBuffer->BindComputePipeline(MotionShader.ComputePipeline);
            cmdBuffer->PushConstantsCompute(&constants, sizeof(constants), 0);
            cmdBuffer->Dispatch(width / 8, height / 8, 1);
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
}

void DebugRenderer::SetVelocityBuffer(Texture::Ptr texture)
{
    VelocityBuffer = texture;
}

void DebugRenderer::Reconstruct()
{
    LineShader.CheckForRebuild(_context);
    MotionShader.CheckForRebuild(_context);
}
