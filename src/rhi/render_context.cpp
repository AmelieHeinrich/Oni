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

    _graphicsFence.Fence = std::make_shared<Fence>(_device);
    _computeFence.Fence = std::make_shared<Fence>(_device);
    _copyFence.Fence = std::make_shared<Fence>(_device);

    _graphicsFence.Value = 0;
    _computeFence.Value = 0;
    _copyFence.Value = 0;

    _swapChain = std::make_shared<SwapChain>(_device, _graphicsQueue, _rtvHeap, hwnd);

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        _commandBuffers[i] = std::make_shared<CommandBuffer>(_device, CommandQueueType::Graphics);
        _frameValues[i] = 0;
    }
}

RenderContext::~RenderContext()
{
    WaitForPreviousFrame();
}

void RenderContext::Resize(uint32_t width, uint32_t height)
{
    WaitForPreviousFrame();

    if (_swapChain) {
        _swapChain->Resize(width, height);
    }
}

void RenderContext::BeginFrame()
{
    _frameIndex = _swapChain->AcquireImage();
    _graphicsFence.Fence->Wait(_frameValues[_frameIndex], 10'000'000);

    _allocator->GetAllocator()->SetCurrentFrameIndex(_frameIndex);
}

void RenderContext::EndFrame()
{
    _frameValues[_frameIndex] = _graphicsFence.Fence->Value();
}

void RenderContext::Present(bool vsync)
{
    _swapChain->Present(vsync);
}

void RenderContext::FlushQueues()
{
    WaitForPreviousHostSubmit(CommandQueueType::Graphics);
    WaitForPreviousHostSubmit(CommandQueueType::Compute);
    WaitForPreviousHostSubmit(CommandQueueType::Copy);
}

void RenderContext::WaitForPreviousFrame()
{
    uint64_t wait = _graphicsFence.Fence->Signal(_graphicsQueue);
    _graphicsFence.Fence->Wait(wait, 10'000'000);
}

void RenderContext::WaitForPreviousHostSubmit(CommandQueueType type)
{
    switch (type) {
        case CommandQueueType::Graphics: {
            _graphicsFence.Fence->Wait(_graphicsFence.Value, 10'000'000);
            break;
        }
        case CommandQueueType::Compute:  {
            _computeFence.Fence->Wait(_computeFence.Value, 10'000'000);
            break;
        }
        case CommandQueueType::Copy: {
            _copyFence.Fence->Wait(_copyFence.Value, 10'000'000);
            break;
        }
    }
}

void RenderContext::WaitForPreviousDeviceSubmit(CommandQueueType type)
{
    switch (type) {
        case CommandQueueType::Graphics: {
            _graphicsQueue->Wait(_graphicsFence.Fence, _graphicsFence.Value);
            break;
        }
        case CommandQueueType::Compute:  {
            _computeQueue->Wait(_computeFence.Fence, _computeFence.Value);
            break;
        }
        case CommandQueueType::Copy: {
            _copyQueue->Wait(_copyFence.Fence, _copyFence.Value);
            break;
        }
    }
}

void RenderContext::ExecuteCommandBuffers(const std::vector<CommandBuffer::Ptr>& buffers, CommandQueueType type)
{
    switch (type) {
        case CommandQueueType::Graphics: {
            _graphicsQueue->Submit(buffers);
            _graphicsFence.Value = _graphicsFence.Fence->Signal(_graphicsQueue);
            break;
        }
        case CommandQueueType::Compute:  {
            _computeQueue->Submit(buffers);
            _computeFence.Value = _computeFence.Fence->Signal(_computeQueue);
            break;
        }
        case CommandQueueType::Copy: {
            _copyQueue->Submit(buffers);
            _copyFence.Value = _copyFence.Fence->Signal(_copyQueue);
            break;
        }
    }
}

CommandBuffer::Ptr RenderContext::GetCurrentCommandBuffer()
{
    return _commandBuffers[_frameIndex];
}

Texture::Ptr RenderContext::GetBackBuffer()
{
    return _swapChain->GetTexture(_frameIndex);
}
