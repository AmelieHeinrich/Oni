/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:06:52
 */

#include "app.hpp"

App::App()
{
    Logger::Init();

    _window = std::make_unique<Window>(1280, 720, "Oni | <D3D12> | <WINDOWS>");
    _window->OnResize([&](uint32_t width, uint32_t height) {
        _renderContext->Resize(width, height);
    });

    _renderContext = std::make_unique<RenderContext>(_window->GetHandle());
}

App::~App()
{
    Logger::Exit();
}

void App::Run()
{
    while (_window->IsOpen()) {
        _renderContext->BeginFrame();

        CommandBuffer::Ptr commandBuffer = _renderContext->GetCurrentCommandBuffer();
        Texture::Ptr texture = _renderContext->GetBackBuffer();

        commandBuffer->Begin();
        commandBuffer->ImageBarrier(texture, TextureLayout::RenderTarget);
        commandBuffer->BindRenderTargets({ texture }, nullptr);
        commandBuffer->ClearRenderTarget(texture, 0.3f, 0.5f, 0.8f, 1.0f);

        // Draw stuff

        commandBuffer->ImageBarrier(texture, TextureLayout::Present);
        commandBuffer->End();
        _renderContext->ExecuteCommandBuffers({ commandBuffer }, CommandQueueType::Graphics);

        _renderContext->EndFrame();
        _renderContext->Present(true);

        _window->Update();
    }
}
