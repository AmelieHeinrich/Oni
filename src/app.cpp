/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:06:52
 */

#include "app.hpp"

App::App()
{
    Logger::Init();

    _window = std::make_unique<Window>(1280, 720, "Oni | <D3D12> | <WINDOWS>");
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

        // Draw stuff

        commandBuffer->ImageBarrier(texture, TextureLayout::Present);
        commandBuffer->End();
        _renderContext->ExecuteCommandBuffers({ commandBuffer }, CommandQueueType::Graphics);

        _renderContext->EndFrame();
        _renderContext->Present(true);

        _window->Update();
    }
}
