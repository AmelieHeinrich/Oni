/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-29 13:02:08
 */

#pragma once

#include "buffer.hpp"
#include "texture.hpp"
#include "descriptor_heap.hpp"
#include "graphics_pipeline.hpp"

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
    
    void SetViewport(float x, float y, float width, float height);
    void SetTopology(Topology topology);

    void ClearRenderTarget(Texture::Ptr renderTarget, float r, float g, float b, float a);

    void BindRenderTargets(const std::vector<Texture::Ptr> renderTargets, Texture::Ptr depthTarget);
    void BindVertexBuffer(Buffer::Ptr buffer);
    void BindIndexBuffer(Buffer::Ptr buffer);
    void BindGraphicsPipeline(GraphicsPipeline::Ptr pipeline);

    void Draw(int vertexCount);
    void DrawIndexed(int indexCount);

    void CopyTextureToTexture(Texture::Ptr dst, Texture::Ptr src);
    void CopyBufferToBuffer(Buffer::Ptr dst, Buffer::Ptr src);

    void BeginImGui();
    void EndImGui();
    void CleanupImGui();

    ID3D12GraphicsCommandList* GetCommandList() { return _commandList; }
private:
    DescriptorHeap::Heaps _heaps;
    D3D12_COMMAND_LIST_TYPE _type;
    ID3D12GraphicsCommandList* _commandList; // TODO(ahi): Switch to newer version of command list to get access to DXR, Mesh shaders and Work Graphs
    ID3D12CommandAllocator* _commandAllocator;
};
