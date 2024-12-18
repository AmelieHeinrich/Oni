/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:50:01
 */

#pragma once

#include <unordered_map>

#include "core/window.hpp"

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
#include "rhi/compute_pipeline.hpp"
#include "rhi/mesh_pipeline.hpp"
#include "rhi/uploader.hpp"
#include "rhi/cube_map.hpp"

#include "rhi/raytracing/acceleration_structure.hpp"
#include "rhi/raytracing/blas.hpp"
#include "rhi/raytracing/tlas.hpp"
#include "rhi/raytracing/raytracing_pipeline.hpp"

struct FencePair
{
    Fence::Ptr Fence;
    uint64_t Value;
};

class RenderContext
{
public:
    using Ptr = std::shared_ptr<RenderContext>;

    RenderContext(std::shared_ptr<Window> hwnd);
    ~RenderContext();

    std::shared_ptr<Window> GetWindow() { return _window; }

    void Resize(uint32_t width, uint32_t height);

    void Present(bool vsync);
    void Finish();

    void WaitForGPU();
    void ExecuteCommandBuffers(const std::vector<CommandBuffer::Ptr>& buffers, CommandQueueType type);

    CommandBuffer::Ptr GetCurrentCommandBuffer();
    Texture::Ptr GetBackBuffer();
    uint32_t GetBackBufferIndex() { return _frameIndex; }

    Buffer::Ptr CreateBuffer(uint64_t size, uint64_t stride, BufferType type, bool readback, const std::string& name = "Buffer");
    Texture::Ptr CreateTexture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, bool mips, const std::string& name = "Texture");
    CubeMap::Ptr CreateCubeMap(uint32_t width, uint32_t height, TextureFormat format, int mips = 1, const std::string& name = "Cube Map");

    GraphicsPipeline::Ptr CreateGraphicsPipeline(GraphicsPipelineSpecs& specs);
    ComputePipeline::Ptr CreateComputePipeline(ShaderBytecode& shader, RootSignature::Ptr rootSignature = nullptr);
    MeshPipeline::Ptr CreateMeshPipeline(GraphicsPipelineSpecs& specs);
    RaytracingPipeline::Ptr CreateRaytracingPipeline(RaytracingPipelineSpecs& specs);
    
    CommandBuffer::Ptr CreateCommandBuffer(CommandQueueType type, bool close = true);
    Sampler::Ptr CreateSampler(SamplerAddress address, SamplerFilter filter, bool mips, int anisotropyLevel);

    BLAS::Ptr CreateBLAS(Buffer::Ptr vertexBuffer, Buffer::Ptr indexBuffer, int vertexCount, int indexCount, const std::string& name = "BLAS");
    TLAS::Ptr CreateTLAS(Buffer::Ptr instanceBuffer, uint32_t numInstances, const std::string& name = "TLAS");
    
    RootSignature::Ptr CreateRootSignature();
    RootSignature::Ptr CreateRootSignature(RootSignatureBuildInfo& info);
    RootSignature::Ptr CreateDefaultRootSignature(uint32_t pushConstantSize);
    
    Uploader CreateUploader();

    void FlushUploader(Uploader& uploader, CommandBuffer::Ptr commandBuffer);
    void FlushUploader(Uploader& uploader);

    void GenerateMips(Texture::Ptr texture);
    void GenerateMips(Texture::Ptr texture, CommandBuffer::Ptr commandBuffer);
    
    void OnGUI();
    void OnOverlay();

    Device::Ptr GetDevice() { return _device; }
private:
    void SetStyle();

    Device::Ptr _device;
    std::shared_ptr<Window> _window;
    
    CommandQueue::Ptr _graphicsQueue;
    FencePair _graphicsFence;

    CommandQueue::Ptr _computeQueue;
    FencePair _computeFence;
    
    CommandQueue::Ptr _copyQueue;
    FencePair _copyFence;

    DescriptorHeap::Heaps _heaps;

    Allocator::Ptr _allocator;

    SwapChain::Ptr _swapChain;
    uint32_t _frameIndex = 0;
    uint64_t _frameValues[FRAMES_IN_FLIGHT];
    CommandBuffer::Ptr _commandBuffers[FRAMES_IN_FLIGHT];

    DescriptorHeap::Descriptor _fontDescriptor;

    ComputePipeline::Ptr _mipmapPipeline;
    Buffer::Ptr _mipmapBuffer;
    Sampler::Ptr _mipmapSampler;

    std::vector<Sampler::Ptr> _samplerCache;
};
