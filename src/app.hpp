/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:05:25
 */

#pragma once

#include "core/log.hpp"
#include "core/window.hpp"

#include "rhi/device.hpp"
#include "rhi/command_queue.hpp"
#include "rhi/fence.hpp"
#include "rhi/descriptor_heap.hpp"

struct FencePair
{
    Fence::Ptr Fence;
    uint64_t Value;
};

class App
{
public:
    App();
    ~App();

    void Run();
private:
    std::unique_ptr<Window> _window;

    Device::Ptr _device;
    
    CommandQueue::Ptr _graphicsQueue;
    FencePair _graphicsFence;

    CommandQueue::Ptr _computeQueue;
    FencePair _computeFence;
    
    CommandQueue::Ptr _copyQueue;
    FencePair _copyFence;

    DescriptorHeap::Ptr _rtvHeap;
    DescriptorHeap::Ptr _dsvHeap;
    DescriptorHeap::Ptr _samplerHeap;
    DescriptorHeap::Ptr _shaderHeap;
};
