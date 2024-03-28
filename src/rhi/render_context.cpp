/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:51:35
 */

#include "render_context.hpp"

RenderContext::RenderContext(HWND hwnd)
{
    _device = std::make_shared<Device>();

    _graphicsQueue = std::make_shared<CommandQueue>(_device, CommandQueueType::Graphics);
    _computeQueue = std::make_shared<CommandQueue>(_device, CommandQueueType::Compute);
    _copyQueue = std::make_shared<CommandQueue>(_device, CommandQueueType::Copy);

    _rtvHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::RenderTarget, 1024);
    _dsvHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::DepthTarget, 1024);
    _shaderHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::ShaderResource, 1'000'000);
    _samplerHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::Sampler, 512);

    _allocator = std::make_shared<Allocator>(_device);

    _swapChain = std::make_shared<SwapChain>(_device, _graphicsQueue, _rtvHeap, hwnd);

    _graphicsFence.Fence = std::make_shared<Fence>(_device);
    _computeFence.Fence = std::make_shared<Fence>(_device);
    _copyFence.Fence = std::make_shared<Fence>(_device);
}

RenderContext::~RenderContext()
{

}

void RenderContext::Resize(uint32_t width, uint32_t height)
{

}
