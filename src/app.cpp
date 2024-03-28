/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:06:52
 */

#include "app.hpp"

App::App()
{
    Logger::Init();

    _window = std::make_unique<Window>(1280, 720, "Oni | <D3D12> | <WINDOWS>");

    _device = std::make_shared<Device>();
    _graphicsQueue = std::make_shared<CommandQueue>(_device, CommandQueueType::Graphics);
    _computeQueue = std::make_shared<CommandQueue>(_device, CommandQueueType::Compute);
    _copyQueue = std::make_shared<CommandQueue>(_device, CommandQueueType::Copy);

    _rtvHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::RenderTarget, 1024);
    _dsvHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::DepthTarget, 1024);
    _shaderHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::ShaderResource, 1'000'000);
    _samplerHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::Sampler, 512);
}

App::~App()
{
    Logger::Exit();
}

void App::Run()
{
    while (_window->IsOpen()) {
        _window->Update();
    }
}
