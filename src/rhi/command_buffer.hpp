/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-29 13:02:08
 */

#pragma once

#include <vector>

#include "core/texture_file.hpp"

#include "device.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "cube_map.hpp"
#include "sampler.hpp"
#include "descriptor_heap.hpp"
#include "graphics_pipeline.hpp"
#include "compute_pipeline.hpp"
#include "mesh_pipeline.hpp"

#include "raytracing/acceleration_structure.hpp"
#include "raytracing/raytracing_pipeline.hpp"
#include "raytracing/tlas.hpp"

#define SUBRESOURCE_ALL 999

enum class CommandQueueType;

enum class Topology
{
    LineList = D3D_PRIMITIVE_TOPOLOGY_LINELIST,
    LineStrip = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
    PointList = D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
    TriangleList = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    TriangleStrip = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
};

struct Barrier
{
    Texture::Ptr Texture;
    TextureLayout NewLayout;
    int Subresource = 0;
};

class CommandBuffer
{
public:
    using Ptr = std::shared_ptr<CommandBuffer>;

    CommandBuffer(Device::Ptr devicePtr, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, CommandQueueType type, bool close = true);
    ~CommandBuffer();

    void Begin(bool reset = true);
    void End();

    void ClearState();

    void BeginEvent(const std::string& name, int r = 255, int g = 255, int b = 255);
    void InsertMarker(const std::string& name, int r = 255, int g = 255, int b = 255);
    void EndEvent();

    void ImageBarrier(Texture::Ptr texture, TextureLayout newLayout, int subresource = SUBRESOURCE_ALL);
    void ImageBarrierBatch(const std::vector<Barrier>& barrier);
    
    void CubeMapBarrier(CubeMap::Ptr cubemap, TextureLayout newLayout);

    void SetViewport(float x, float y, float width, float height);
    void SetTopology(Topology topology);

    void ClearRenderTarget(Texture::Ptr renderTarget, float r, float g, float b, float a);
    void ClearDepthTarget(Texture::Ptr depthTarget);
    void ClearUAV(Texture::Ptr texture, float r, float g, float b, float a, int subresource = 0);

    void BindRenderTargets(const std::vector<Texture::Ptr>& renderTargets, Texture::Ptr depthTarget);
    void BindVertexBuffer(Buffer::Ptr buffer);
    void BindIndexBuffer(Buffer::Ptr buffer);

    void BindMeshPipeline(MeshPipeline::Ptr pipeline);

    void BindRaytracingPipeline(RaytracingPipeline::Ptr pipeline);

    void BindGraphicsPipeline(GraphicsPipeline::Ptr pipeline);
    void BindGraphicsConstantBuffer(Buffer::Ptr buffer, int index);
    void BindGraphicsShaderResource(Texture::Ptr texture, int index);
    void BindGraphicsSampler(Sampler::Ptr sampler, int index);
    void BindGraphicsCubeMap(CubeMap::Ptr cubemap, int index);

    void BindComputePipeline(ComputePipeline::Ptr pipeline);
    void BindComputeShaderResource(Texture::Ptr texture, int index, int mip = 0);
    void BindComputeStorageTexture(Texture::Ptr texture, int index, int mip = 0);
    void BindComputeAccelerationStructure(TLAS::Ptr tlas, int index);
    void BindComputeCubeMapShaderResource(CubeMap::Ptr texture, int index);
    void BindComputeCubeMapStorage(CubeMap::Ptr texture, int index, int mip = 0);
    void BindComputeConstantBuffer(Buffer::Ptr buffer, int index);
    void BindComputeStorageBuffer(Buffer::Ptr buffer, int index);
    void BindComputeSampler(Sampler::Ptr sampler, int index);

    void PushConstantsGraphics(const void *data, uint32_t size, int index);
    void PushConstantsCompute(const void *data, uint32_t size, int index);

    void Draw(int vertexCount);
    void DrawIndexed(int indexCount);
    void Dispatch(int x, int y, int z);
    void DispatchMesh(int x, int y, int z);
    void TraceRays(int width, int height);

    void CopyTextureToTexture(Texture::Ptr dst, Texture::Ptr src);
    void CopyBufferToBuffer(Buffer::Ptr dst, Buffer::Ptr src);
    void CopyBufferToTexture(Texture::Ptr dst, Buffer::Ptr src);
    void CopyTextureToBuffer(Buffer::Ptr dst, Texture::Ptr src);

    void CopyBufferToTextureLOD(Texture::Ptr dst, Buffer::Ptr src, int mip);
    void CopyTextureFileToTexture(Texture::Ptr dst, Buffer::Ptr srcTexels, TextureFile *file);

    // RT
    void BuildAccelerationStructure(AccelerationStructure structure, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs);
    //

    void BeginImGui(int width, int height);
    void EndImGui();
    void CleanupImGui();

    ID3D12GraphicsCommandList* GetCommandList() { return _commandList; }
private:
    Allocator::Ptr _allocator;
    Device::Ptr _device;
    DescriptorHeap::Heaps _heaps;
    D3D12_COMMAND_LIST_TYPE _type;
    ID3D12GraphicsCommandList6* _commandList; // TODO(ahi): Switch to newer version of command list to get access to DXR, Mesh shaders and Work Graphs
    ID3D12CommandAllocator* _commandAllocator;

    RaytracingPipeline::Ptr _currentlyBoundRT = nullptr;
};
