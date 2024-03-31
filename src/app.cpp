/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:06:52
 */

#include "app.hpp"

#include "shader/bytecode.hpp"
#include "core/image.hpp"

#include "model.hpp"

#include <ImGui/imgui.h>
#include <glm/gtc/type_ptr.hpp>

App::App()
    : _camera(1280, 720), _lastFrame(0.0f)
{
    Logger::Init();

    _window = std::make_unique<Window>(1280, 720, "Oni | <D3D12> | <WINDOWS>");
    _window->OnResize([&](uint32_t width, uint32_t height) {
        _renderContext->Resize(width, height);
        _camera.Resize(width, height);
        _depthBuffer.reset();
        _depthBuffer = _renderContext->CreateTexture(width, height, TextureFormat::R32Depth, TextureUsage::DepthTarget);
        _depthBuffer->BuildDepthTarget();
    });

    _renderContext = std::make_shared<RenderContext>(_window->GetHandle());

    _depthBuffer = _renderContext->CreateTexture(1280, 720, TextureFormat::R32Depth, TextureUsage::DepthTarget);
    _depthBuffer->BuildDepthTarget();

    GraphicsPipelineSpecs specs;
    specs.FormatCount = 1;
    specs.Formats[0] = TextureFormat::RGBA8;
    specs.DepthFormat = TextureFormat::R32Depth;
    specs.Depth = DepthOperation::Less;
    specs.DepthEnabled = true;
    specs.Cull = CullMode::None;
    specs.Fill = FillMode::Solid;
    ShaderCompiler::CompileShader("shaders/HelloTriangle/TriVertex.hlsl", "Main", ShaderType::Vertex, specs.Bytecodes[ShaderType::Vertex]);
    ShaderCompiler::CompileShader("shaders/HelloTriangle/TriFrag.hlsl", "Main", ShaderType::Fragment, specs.Bytecodes[ShaderType::Fragment]);
    
    _triPipeline = _renderContext->CreateGraphicsPipeline(specs);
    
    _sceneBuffer = _renderContext->CreateBuffer(256, 0, BufferType::Constant, false);
    _sceneBuffer->BuildConstantBuffer();
    
    _modelBuffer = _renderContext->CreateBuffer(256, 0, BufferType::Constant, false);
    _modelBuffer->BuildConstantBuffer();

    _sampler = _renderContext->CreateSampler(SamplerAddress::Wrap, SamplerFilter::Linear, 0);

    // TEST MODEL
    _model.Load(_renderContext, "assets/models/Sponza.gltf");
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
        _sceneBuffer->Map(0, 0, &pData);
        memcpy(pData, &buffer, sizeof(SceneBuffer));
        _sceneBuffer->Unmap(0, 0);

        commandBuffer->Begin();
        commandBuffer->ImageBarrier(texture, TextureLayout::RenderTarget);
        commandBuffer->SetViewport(0, 0, width, height);
        commandBuffer->SetTopology(Topology::TriangleList);
        commandBuffer->BindRenderTargets({ texture }, _depthBuffer);
        commandBuffer->ClearRenderTarget(texture, 0.3f, 0.5f, 0.8f, 1.0f);
        commandBuffer->ClearDepthTarget(_depthBuffer);
        commandBuffer->BindGraphicsPipeline(_triPipeline);
        commandBuffer->BindGraphicsConstantBuffer(_sceneBuffer, 0);
        commandBuffer->BindGraphicsSampler(_sampler, 3);

        for (auto& primitive : _model.Primitives) {
            auto& material = _model.Materials[primitive.MaterialIndex];

            _modelBuffer->Map(0, 0, &pData);
            memcpy(pData, glm::value_ptr(primitive.Transform), sizeof(glm::mat4));
            _modelBuffer->Unmap(0, 0);

            commandBuffer->BindVertexBuffer(primitive.VertexBuffer);
            commandBuffer->BindIndexBuffer(primitive.IndexBuffer);
            commandBuffer->BindGraphicsConstantBuffer(_modelBuffer, 1);
            commandBuffer->BindGraphicsShaderResource(material.AlbedoTexture, 2);
            commandBuffer->DrawIndexed(primitive.IndexCount);
        }

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
