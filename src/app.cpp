/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:06:52
 */

#include "app.hpp"

#include "shader/bytecode.hpp"

#include <ImGui/imgui.h>

App::App()
{
    Logger::Init();

    _window = std::make_unique<Window>(1280, 720, "Oni | <D3D12> | <WINDOWS>");
    _window->OnResize([&](uint32_t width, uint32_t height) {
        _renderContext->Resize(width, height);
    });

    _renderContext = std::make_unique<RenderContext>(_window->GetHandle());

    GraphicsPipelineSpecs specs;
    specs.FormatCount = 1;
    specs.Formats[0] = TextureFormat::RGBA8;
    specs.DepthEnabled = false;
    specs.Cull = CullMode::Back;
    specs.Fill = FillMode::Solid;
    ShaderCompiler::CompileShader("shaders/HelloTriangle/TriVertex.hlsl", "Main", ShaderType::Vertex, specs.Bytecodes[ShaderType::Vertex]);
    ShaderCompiler::CompileShader("shaders/HelloTriangle/TriFrag.hlsl", "Main", ShaderType::Fragment, specs.Bytecodes[ShaderType::Fragment]);
    
    _triPipeline = _renderContext->CreateGraphicsPipeline(specs);

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    _vertexBuffer = _renderContext->CreateBuffer(sizeof(vertices), sizeof(float) * 3, BufferType::Vertex, false);

    Uploader uploader = _renderContext->CreateUploader();
    uploader.CopyHostToDeviceLocal(vertices, sizeof(vertices), _vertexBuffer);
    _renderContext->FlushUploader(uploader);
}

App::~App()
{
    Logger::Exit();
}

void App::Run()
{
    while (_window->IsOpen()) {
        uint32_t width, height;
        _window->GetSize(width, height);

        _renderContext->BeginFrame();

        CommandBuffer::Ptr commandBuffer = _renderContext->GetCurrentCommandBuffer();
        Texture::Ptr texture = _renderContext->GetBackBuffer();

        commandBuffer->Begin();
        commandBuffer->ImageBarrier(texture, TextureLayout::RenderTarget);

        commandBuffer->SetViewport(0, 0, width, height);
        commandBuffer->SetTopology(Topology::TriangleList);

        commandBuffer->BindRenderTargets({ texture }, nullptr);
        commandBuffer->BindGraphicsPipeline(_triPipeline);
        commandBuffer->BindVertexBuffer(_vertexBuffer);

        commandBuffer->ClearRenderTarget(texture, 0.3f, 0.5f, 0.8f, 1.0f);

        commandBuffer->Draw(3);

        commandBuffer->BeginImGui();
        RenderOverlay();
        commandBuffer->EndImGui();

        commandBuffer->ImageBarrier(texture, TextureLayout::Present);
        commandBuffer->End();
        _renderContext->ExecuteCommandBuffers({ commandBuffer }, CommandQueueType::Graphics);

        _renderContext->EndFrame();
        _renderContext->Present(true);

        _window->Update();
    }
}

void App::RenderOverlay()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                _window->Close();
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}
