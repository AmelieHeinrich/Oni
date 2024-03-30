/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:06:52
 */

#include "app.hpp"

#include "shader/bytecode.hpp"
#include "core/image.hpp"

#include <ImGui/imgui.h>

App::App()
    : _camera(1280, 720), _lastFrame(0.0f)
{
    Logger::Init();

    _window = std::make_unique<Window>(1280, 720, "Oni | <D3D12> | <WINDOWS>");
    _window->OnResize([&](uint32_t width, uint32_t height) {
        _renderContext->Resize(width, height);
        _camera.Resize(width, height);
    });

    _renderContext = std::make_unique<RenderContext>(_window->GetHandle());

    GraphicsPipelineSpecs specs;
    specs.FormatCount = 1;
    specs.Formats[0] = TextureFormat::RGBA8;
    specs.DepthEnabled = false;
    specs.Cull = CullMode::None;
    specs.Fill = FillMode::Solid;
    ShaderCompiler::CompileShader("shaders/HelloTriangle/TriVertex.hlsl", "Main", ShaderType::Vertex, specs.Bytecodes[ShaderType::Vertex]);
    ShaderCompiler::CompileShader("shaders/HelloTriangle/TriFrag.hlsl", "Main", ShaderType::Fragment, specs.Bytecodes[ShaderType::Fragment]);
    
    _triPipeline = _renderContext->CreateGraphicsPipeline(specs);

    float vertices[] = {
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
    };

    uint32_t indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    Image image;
    image.LoadFromFile("assets/textures/texture.jpg");

    _vertexBuffer = _renderContext->CreateBuffer(sizeof(vertices), sizeof(float) * 5, BufferType::Vertex, false);
    _indexBuffer = _renderContext->CreateBuffer(sizeof(indices), sizeof(uint32_t), BufferType::Index, false);
    
    _constantBuffer = _renderContext->CreateBuffer(256, 0, BufferType::Constant, false);
    _constantBuffer->BuildConstantBuffer();

    _texture = _renderContext->CreateTexture(image.Width, image.Height, TextureFormat::RGBA8, TextureUsage::ShaderResource);
    _texture->BuildShaderResource();
    
    _sampler = _renderContext->CreateSampler(SamplerAddress::Clamp, SamplerFilter::Linear, 0);

    Uploader uploader = _renderContext->CreateUploader();
    uploader.CopyHostToDeviceLocal(vertices, sizeof(vertices), _vertexBuffer);
    uploader.CopyHostToDeviceLocal(indices, sizeof(indices), _indexBuffer);
    uploader.CopyHostToDeviceTexture(image, _texture);
    _renderContext->FlushUploader(uploader);
}

App::~App()
{
    _renderContext->WaitForPreviousDeviceSubmit(CommandQueueType::Graphics);
    _renderContext->WaitForPreviousHostSubmit(CommandQueueType::Graphics);

    Logger::Exit();
}

void App::Run()
{
    while (_window->IsOpen()) {
        _window->Update();

        uint32_t width, height;
        _window->GetSize(width, height);

        float time = _dtTimer.GetElapsed();
        float dt = (time - _lastFrame) / 1000.0f;
        _lastFrame = time;

        _camera.Update(dt);

        SceneBuffer buffer;
        buffer.View = _camera.View();
        buffer.Projection = _camera.Projection();

        _renderContext->BeginFrame();

        CommandBuffer::Ptr commandBuffer = _renderContext->GetCurrentCommandBuffer();
        Texture::Ptr texture = _renderContext->GetBackBuffer();

        void *pData;
        _constantBuffer->Map(0, 0, &pData);
        memcpy(pData, &buffer, sizeof(SceneBuffer));
        _constantBuffer->Unmap(0, 0);

        commandBuffer->Begin();
        commandBuffer->ImageBarrier(texture, TextureLayout::RenderTarget);
        commandBuffer->SetViewport(0, 0, width, height);
        commandBuffer->SetTopology(Topology::TriangleList);
        commandBuffer->BindRenderTargets({ texture }, nullptr);
        commandBuffer->BindGraphicsPipeline(_triPipeline);
        commandBuffer->BindVertexBuffer(_vertexBuffer);
        commandBuffer->BindIndexBuffer(_indexBuffer);
        commandBuffer->BindGraphicsConstantBuffer(_constantBuffer, 0);
        commandBuffer->BindGraphicsShaderResource(_texture, 1);
        commandBuffer->BindGraphicsSampler(_sampler, 2);
        commandBuffer->ClearRenderTarget(texture, 0.3f, 0.5f, 0.8f, 1.0f);
        commandBuffer->DrawIndexed(6);

        commandBuffer->BeginImGui();
        RenderOverlay();
        commandBuffer->EndImGui();

        commandBuffer->ImageBarrier(texture, TextureLayout::Present);
        commandBuffer->End();
        _renderContext->ExecuteCommandBuffers({ commandBuffer }, CommandQueueType::Graphics);
        _renderContext->EndFrame();
        _renderContext->Present(true);

        _camera.Input(dt);
    }
}

void App::RenderOverlay()
{
    
}
