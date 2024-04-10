/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-29 13:02:08
 */

#pragma once

#include "buffer.hpp"
#include "texture.hpp"
#include "cube_map.hpp"
#include "sampler.hpp"
#include "descriptor_heap.hpp"
#include "graphics_pipeline.hpp"
#include "compute_pipeline.hpp"

enum class CommandQueueType;

enum class Topology
{
    LineList = D3D_PRIMITIVE_TOPOLOGY_LINELIST,
    LineStrip = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
    PointList = D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
    TriangleList = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    TriangleStrip = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
};

class CommandBuffer
{
public:
    using Ptr = std::shared_ptr<CommandBuffer>;

    CommandBuffer(Device::Ptr devicePtr, DescriptorHeap::Heaps& heaps, CommandQueueType type);
    ~CommandBuffer();

    void Begin();
    void End();

    void ImageBarrier(Texture::Ptr texture, TextureLayout newLayout);
    void CubeMapBarrier(CubeMap::Ptr cubemap, TextureLayout newLayout);
    
    void SetViewport(float x, float y, float width, float height);
    void SetTopology(Topology topology);

    void ClearRenderTarget(Texture::Ptr renderTarget, float r, float g, float b, float a);
    void ClearDepthTarget(Texture::Ptr depthTarget);

    void BindRenderTargets(const std::vector<Texture::Ptr> renderTargets, Texture::Ptr depthTarget);
    void BindVertexBuffer(Buffer::Ptr buffer);
    void BindIndexBuffer(Buffer::Ptr buffer);

    void BindGraphicsPipeline(GraphicsPipeline::Ptr pipeline);
    void BindGraphicsConstantBuffer(Buffer::Ptr buffer, int index);
    void BindGraphicsShaderResource(Texture::Ptr texture, int index);
    void BindGraphicsSampler(Sampler::Ptr sampler, int index);
    void BindGraphicsCubeMap(CubeMap::Ptr cubemap, int index);

    void BindComputePipeline(ComputePipeline::Ptr pipeline);
    void BindComputeShaderResource(Texture::Ptr texture, int index, int mip);
    void BindComputeStorageTexture(Texture::Ptr texture, int index, int mip);
    void BindComputeCubeMapShaderResource(CubeMap::Ptr texture, int index);
    void BindComputeCubeMapStorage(CubeMap::Ptr texture, int index, int mip);
    void BindComputeConstantBuffer(Buffer::Ptr buffer, int index);
    void BindComputeSampler(Sampler::Ptr sampler, int index);

    void Draw(int vertexCount);
    void DrawIndexed(int indexCount);
    void Dispatch(int x, int y, int z);

    void CopyTextureToTexture(Texture::Ptr dst, Texture::Ptr src);
    void CopyBufferToBuffer(Buffer::Ptr dst, Buffer::Ptr src);
    void CopyBufferToTexture(Texture::Ptr dst, Buffer::Ptr src);
    void CopyTextureToBuffer(Buffer::Ptr dst, Texture::Ptr src);

    void BeginImGui(int width, int height);
    void EndImGui();
    void CleanupImGui();

    ID3D12GraphicsCommandList* GetCommandList() { return _commandList; }
private:
    void ImageBarrier(Texture::Ptr texture, D3D12_RESOURCE_STATES state);

    DescriptorHeap::Heaps _heaps;
    D3D12_COMMAND_LIST_TYPE _type;
    ID3D12GraphicsCommandList* _commandList; // TODO(ahi): Switch to newer version of command list to get access to DXR, Mesh shaders and Work Graphs
    ID3D12CommandAllocator* _commandAllocator;
};
