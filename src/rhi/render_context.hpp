/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:50:01
 */

#pragma once

#include "rhi/device.hpp"
#include "rhi/command_queue.hpp"
#include "rhi/fence.hpp"
#include "rhi/descriptor_heap.hpp"
#include "rhi/allocator.hpp"
#include "rhi/swap_chain.hpp"
#include "rhi/texture.hpp"
#include "rhi/buffer.hpp"
#include "rhi/command_buffer.hpp"
#include "rhi/graphics_pipeline.hpp"
#include "rhi/uploader.hpp"

struct FencePair
{
    Fence::Ptr Fence;
    uint64_t Value;
};

class RenderContext
{
public:
    using Ptr = std::unique_ptr<RenderContext>;

    RenderContext(HWND hwnd);
    ~RenderContext();

    void Resize(uint32_t width, uint32_t height);

    void BeginFrame();
    void EndFrame();

    void Present(bool vsync);

    void WaitForPreviousHostSubmit(CommandQueueType type);
    void WaitForPreviousDeviceSubmit(CommandQueueType type);
    void ExecuteCommandBuffers(const std::vector<CommandBuffer::Ptr>& buffers, CommandQueueType type);

    CommandBuffer::Ptr GetCurrentCommandBuffer();
    Texture::Ptr GetBackBuffer();

    Buffer::Ptr CreateBuffer(uint64_t size, uint64_t stride, BufferType type, bool readback);
    GraphicsPipeline::Ptr CreateGraphicsPipeline(GraphicsPipelineSpecs& specs);
    Texture::Ptr CreateTexture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage);
    Sampler::Ptr CreateSampler(SamplerAddress address, SamplerFilter filter, int anisotropyLevel);
    
    Uploader CreateUploader();
    void FlushUploader(Uploader& uploader);
private:
    void FlushQueues();
    void WaitForPreviousFrame();

private:
    Device::Ptr _device;
    
    CommandQueue::Ptr _graphicsQueue;
    FencePair _graphicsFence;

    CommandQueue::Ptr _computeQueue;
    FencePair _computeFence;
    
    CommandQueue::Ptr _copyQueue;
    FencePair _copyFence;

    DescriptorHeap::Heaps _heaps;

    Allocator::Ptr _allocator;

    SwapChain::Ptr _swapChain;
    uint32_t _frameIndex;
    uint64_t _frameValues[FRAMES_IN_FLIGHT];
    CommandBuffer::Ptr _commandBuffers[FRAMES_IN_FLIGHT];

    DescriptorHeap::Descriptor _fontDescriptor;
};
