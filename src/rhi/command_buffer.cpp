/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-29 13:04:28
 */

#include "command_buffer.hpp"
#include "command_queue.hpp"

#include <core/log.hpp>

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx12.h>

#include <sstream>
#include <ctime>
#include <algorithm>
#include <vector>

#include <pix3.h>

#undef max

bool IsHDR(TextureFormat format)
{
    if (format == TextureFormat::RGBA32Float)
        return true;
    return false;
}

CommandBuffer::CommandBuffer(Device::Ptr devicePtr, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, CommandQueueType type, bool close)
    : _type(D3D12_COMMAND_LIST_TYPE(type)), _heaps(heaps), _device(devicePtr), _allocator(allocator)
{
    HRESULT result = devicePtr->GetDevice()->CreateCommandAllocator(_type, IID_PPV_ARGS(&_commandAllocator));
    if (FAILED(result)) {
        Logger::Error("[D3D12] Failed to create command allocator!");
    }

    result = devicePtr->GetDevice()->CreateCommandList(0, _type, _commandAllocator, nullptr, IID_PPV_ARGS(&_commandList));
    if (FAILED(result)) {
        Logger::Error("[D3D12] Failed to create command list!");
    }

    if (close) {
        result = _commandList->Close();
        if (FAILED(result)) {
            Logger::Error("[D3D12] Failed to close freshly created command list!");
        }
    }
}

CommandBuffer::~CommandBuffer()
{
    _commandList->Release();
    _commandAllocator->Release();
}

void CommandBuffer::Begin(bool reset)
{
    if (reset) {
        _commandAllocator->Reset();
        _commandList->Reset(_commandAllocator, nullptr);
    }

    if (_type == D3D12_COMMAND_LIST_TYPE_DIRECT || _type == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        ID3D12DescriptorHeap* heaps[] = {
            _heaps.ShaderHeap->GetHeap(),
            _heaps.SamplerHeap->GetHeap()
        };
        _commandList->SetDescriptorHeaps(2, heaps);
    }
}

void CommandBuffer::End()
{
    _commandList->Close();
}

void CommandBuffer::ClearState()
{
    _commandList->ClearState(nullptr);
}

void CommandBuffer::BeginEvent(const std::string& name, int r, int g, int b)
{
    PIXBeginEvent(_commandList, PIX_COLOR(r, g, b), name.c_str());
}

void CommandBuffer::InsertMarker(const std::string& name, int r, int g, int b)
{
    PIXSetMarker(_commandList, PIX_COLOR(r, g, b), name.c_str());
}

void CommandBuffer::EndEvent()
{
    PIXEndEvent(_commandList);
}

void CommandBuffer::ImageBarrier(Texture::Ptr texture, TextureLayout newLayout, int subresource)
{
    TextureLayout oldLayout = TextureLayout(texture->GetState(subresource));

    if (oldLayout == TextureLayout::Storage && newLayout == TextureLayout::Storage) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = texture->GetResource().Resource;

        _commandList->ResourceBarrier(1, &barrier);
        texture->SetState(D3D12_RESOURCE_STATES(newLayout), subresource);
    } else {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATES(oldLayout);
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATES(newLayout);
        barrier.Transition.pResource = texture->_resource->Resource;
        if (subresource == SUBRESOURCE_ALL) {
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        } else {
            barrier.Transition.Subresource = subresource;
        }

        if (barrier.Transition.StateBefore == barrier.Transition.StateAfter)
            return;
    
        _commandList->ResourceBarrier(1, &barrier);
        texture->SetState(D3D12_RESOURCE_STATES(newLayout), subresource);
    }
}

void CommandBuffer::ImageBarrierBatch(const std::vector<Barrier>& barrier)
{
    std::vector<D3D12_RESOURCE_BARRIER> barrierList;
    for (int i = 0; i < barrier.size(); i++) {
        D3D12_RESOURCE_BARRIER pushBarrier = {};
        TextureLayout oldLayout = TextureLayout(barrier[i].Texture->GetState(barrier[i].Subresource));

        if (barrier[i].NewLayout == TextureLayout::Storage && oldLayout == TextureLayout::Storage) {
            pushBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            pushBarrier.UAV.pResource = barrier[i].Texture->GetResource().Resource;

            barrier[i].Texture->SetState(D3D12_RESOURCE_STATES(barrier[i].NewLayout), barrier[i].Subresource);
            barrierList.push_back(pushBarrier);
        } else {
            pushBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            pushBarrier.Transition.pResource = barrier[i].Texture->GetResource().Resource;
            pushBarrier.Transition.StateBefore = barrier[i].Texture->GetState(barrier[i].Subresource);
            pushBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES(barrier[i].NewLayout);
            pushBarrier.Transition.Subresource = barrier[i].Subresource;

            if (pushBarrier.Transition.StateBefore == pushBarrier.Transition.StateAfter)
                continue;

            barrier[i].Texture->SetState(D3D12_RESOURCE_STATES(barrier[i].NewLayout), barrier[i].Subresource);
            barrierList.push_back(pushBarrier);
        }
    }
    if (barrierList.size() == 0) {
        return;
    }
    _commandList->ResourceBarrier(barrierList.size(), barrierList.data());
}

void CommandBuffer::CubeMapBarrier(CubeMap::Ptr cubemap, TextureLayout newLayout)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = cubemap->GetResource().Resource;
    barrier.Transition.StateBefore = cubemap->GetState();
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATES(newLayout);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    if (barrier.Transition.StateBefore == barrier.Transition.StateAfter)
        return;

    _commandList->ResourceBarrier(1, &barrier);

    cubemap->SetState(D3D12_RESOURCE_STATES(newLayout));
}

void CommandBuffer::SetViewport(float x, float y, float width, float height)
{
    D3D12_VIEWPORT Viewport = {};
    Viewport.Width = width;
    Viewport.Height = height;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    Viewport.TopLeftX = x;
    Viewport.TopLeftY = y;

    D3D12_RECT Rect;
    Rect.right = width;
    Rect.bottom = height;
    Rect.top = 0.0f;
    Rect.left = 0.0f;

    _commandList->RSSetViewports(1, &Viewport);
    _commandList->RSSetScissorRects(1, &Rect);
}

void CommandBuffer::SetTopology(Topology topology)
{
    _commandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY(topology));
}

void CommandBuffer::BindRenderTargets(const std::vector<Texture::Ptr>& renderTargets, Texture::Ptr depthTarget)
{
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvDescriptors;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor;

    for (auto& renderTarget : renderTargets) {
        rtvDescriptors.push_back(renderTarget->_rtv.CPU);
    }
    if (depthTarget) {
        dsvDescriptor = depthTarget->_dsv.CPU;
    }

    _commandList->OMSetRenderTargets(rtvDescriptors.size(), rtvDescriptors.data(), false, depthTarget ? &dsvDescriptor : nullptr);
}

void CommandBuffer::ClearRenderTarget(Texture::Ptr renderTarget, float r, float g, float b, float a)
{
    float clearValues[4] = { r, g, b, a };
    _commandList->ClearRenderTargetView(renderTarget->_rtv.CPU, clearValues, 0, nullptr);
}

void CommandBuffer::ClearDepthTarget(Texture::Ptr depthTarget)
{
    _commandList->ClearDepthStencilView(depthTarget->_dsv.CPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void CommandBuffer::ClearUAV(Texture::Ptr texture, float r, float g, float b, float a, int subresource)
{
    float clearValues[4] = { r, g, b, a };
    _commandList->ClearUnorderedAccessViewFloat(texture->_uavs[subresource].GPU, texture->_uavs[subresource].CPU, texture->_resource->Resource, clearValues, 0, nullptr);
}

void CommandBuffer::BindVertexBuffer(Buffer::Ptr buffer)
{
    _commandList->IASetVertexBuffers(0, 1, &buffer->_VBV);
}

void CommandBuffer::BindIndexBuffer(Buffer::Ptr buffer)
{
    _commandList->IASetIndexBuffer(&buffer->_IBV);
}

void CommandBuffer::BindMeshPipeline(MeshPipeline::Ptr pipeline)
{
    _commandList->SetPipelineState(pipeline->GetPipeline());
    _commandList->SetGraphicsRootSignature(pipeline->GetSignature()->GetSignature());
}

void CommandBuffer::BindRaytracingPipeline(RaytracingPipeline::Ptr pipeline)
{
    _currentlyBoundRT = pipeline;

    _commandList->SetPipelineState1(pipeline->GetPipeline());
    _commandList->SetComputeRootSignature(pipeline->GetSignature()->GetSignature());
}

void CommandBuffer::BindGraphicsPipeline(GraphicsPipeline::Ptr pipeline)
{
    _commandList->SetPipelineState(pipeline->GetPipeline());
    _commandList->SetGraphicsRootSignature(pipeline->GetSignature()->GetSignature());
}

void CommandBuffer::BindGraphicsConstantBuffer(Buffer::Ptr buffer, int index)
{
    _commandList->SetGraphicsRootDescriptorTable(index, buffer->_cbv.GPU);
}

void CommandBuffer::BindGraphicsShaderResource(Texture::Ptr texture, int index)
{
    _commandList->SetGraphicsRootDescriptorTable(index, texture->_srvs[0].GPU);
}

void CommandBuffer::BindGraphicsSampler(Sampler::Ptr sampler, int index)
{
    _commandList->SetGraphicsRootDescriptorTable(index, sampler->GetDescriptor().GPU);
}

void CommandBuffer::BindGraphicsCubeMap(CubeMap::Ptr cubemap, int index)
{
    _commandList->SetGraphicsRootDescriptorTable(index, cubemap->_srv.GPU);
}

void CommandBuffer::BindComputePipeline(ComputePipeline::Ptr pipeline)
{
    _commandList->SetPipelineState(pipeline->GetPipeline());
    _commandList->SetComputeRootSignature(pipeline->GetSignature()->GetSignature());
}

void CommandBuffer::BindComputeShaderResource(Texture::Ptr texture, int index, int mip)
{
    _commandList->SetComputeRootDescriptorTable(index, texture->_srvs[mip].GPU);
}

void CommandBuffer::BindComputeStorageTexture(Texture::Ptr texture, int index, int mip)
{
    _commandList->SetComputeRootDescriptorTable(index, texture->_uavs[mip].GPU);
}

void CommandBuffer::BindComputeCubeMapShaderResource(CubeMap::Ptr texture, int index)
{
    _commandList->SetComputeRootDescriptorTable(index, texture->_srv.GPU);
}

void CommandBuffer::BindComputeCubeMapStorage(CubeMap::Ptr texture, int index, int mip)
{
    _commandList->SetComputeRootDescriptorTable(index, texture->_uavs[mip].GPU);
}

void CommandBuffer::BindComputeAccelerationStructure(TLAS::Ptr tlas, int index)
{
    _commandList->SetComputeRootDescriptorTable(index, tlas->_srv.GPU);
}

void CommandBuffer::BindComputeConstantBuffer(Buffer::Ptr buffer, int index)
{
    _commandList->SetComputeRootDescriptorTable(index, buffer->_cbv.GPU);
}

void CommandBuffer::BindComputeStorageBuffer(Buffer::Ptr buffer, int index)
{
    _commandList->SetComputeRootDescriptorTable(index, buffer->_uav.GPU);
}

void CommandBuffer::BindComputeSampler(Sampler::Ptr sampler, int index)
{
    _commandList->SetComputeRootDescriptorTable(index, sampler->GetDescriptor().GPU);
}

void CommandBuffer::PushConstantsGraphics(const void *data, uint32_t size, int index)
{
    _commandList->SetGraphicsRoot32BitConstants(index, size / 4, data, 0);
}

void CommandBuffer::PushConstantsCompute(const void *data, uint32_t size, int index)
{
    _commandList->SetComputeRoot32BitConstants(index, size / 4, data, 0);
}

void CommandBuffer::Draw(int vertexCount)
{
    _commandList->DrawInstanced(vertexCount, 1, 0, 0);
}

void CommandBuffer::DrawIndexed(int indexCount)
{
    _commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void CommandBuffer::Dispatch(int x, int y, int z)
{
    _commandList->Dispatch(x, y, z);
}

void CommandBuffer::DispatchMesh(int x, int y, int z)
{
    _commandList->DispatchMesh(x, y, z);
}

void CommandBuffer::TraceRays(int width, int height)
{
    if (_currentlyBoundRT == nullptr) {
        Logger::Error("Please bind a raytracing pipeline before calling TraceRays");
    }
    uint64_t address = _currentlyBoundRT->GetTables()->Address();

    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
    dispatchDesc.RayGenerationShaderRecord.StartAddress = address;
    dispatchDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    dispatchDesc.MissShaderTable.StartAddress = address + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    dispatchDesc.MissShaderTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    dispatchDesc.HitGroupTable.StartAddress = address + (2 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    dispatchDesc.HitGroupTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    dispatchDesc.Width = width;
    dispatchDesc.Height = height;
    dispatchDesc.Depth = 1;

    _commandList->DispatchRays(&dispatchDesc);
}

void CommandBuffer::CopyTextureToTexture(Texture::Ptr dst, Texture::Ptr src)
{
    D3D12_TEXTURE_COPY_LOCATION BlitSource = {};
    BlitSource.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    BlitSource.pResource = src->GetResource().Resource;
    BlitSource.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION BlitDest = {};
    BlitDest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    BlitDest.pResource = dst->GetResource().Resource;
    BlitDest.SubresourceIndex = 0;

    _commandList->CopyTextureRegion(&BlitDest, 0, 0, 0, &BlitSource, nullptr);
}

void CommandBuffer::CopyBufferToBuffer(Buffer::Ptr dst, Buffer::Ptr src)
{
    _commandList->CopyResource(dst->_resource->Resource, src->_resource->Resource);
}

void CommandBuffer::CopyBufferToTexture(Texture::Ptr dst, Buffer::Ptr src)
{
    D3D12_TEXTURE_COPY_LOCATION CopySource = {};
    CopySource.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    CopySource.pResource = src->_resource->Resource;
    CopySource.PlacedFootprint.Offset = 0;
    CopySource.PlacedFootprint.Footprint.Format = DXGI_FORMAT(dst->_format);
    CopySource.PlacedFootprint.Footprint.Width = dst->_width;
    CopySource.PlacedFootprint.Footprint.Height = dst->_height;
    CopySource.PlacedFootprint.Footprint.Depth = 1;
    CopySource.PlacedFootprint.Footprint.RowPitch = dst->_width * Texture::GetComponentSize(dst->GetFormat());
    CopySource.SubresourceIndex = 0;

    if (dst->GetFormat() == TextureFormat::BC1) {
        CopySource.PlacedFootprint.Footprint.RowPitch = dst->_width * 2;
    } else if (dst->GetFormat() == TextureFormat::BC7) {
        CopySource.PlacedFootprint.Footprint.RowPitch = dst->_width * 4;
    }

    D3D12_TEXTURE_COPY_LOCATION CopyDest = {};
    CopyDest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    CopyDest.pResource = dst->_resource->Resource;
    CopyDest.SubresourceIndex = 0;

    _commandList->CopyTextureRegion(&CopyDest, 0, 0, 0, &CopySource, nullptr);
}

void CommandBuffer::CopyBufferToTextureLOD(Texture::Ptr dst, Buffer::Ptr src, int mip)
{
    int width = mip > 0 ? dst->GetSizeOfMip(mip) : dst->_width;
    int height = mip > 0 ? dst->GetSizeOfMip(mip) : dst->_height;

    D3D12_TEXTURE_COPY_LOCATION CopySource = {};
    CopySource.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    CopySource.pResource = src->_resource->Resource;
    CopySource.PlacedFootprint.Offset = 0;
    CopySource.PlacedFootprint.Footprint.Format = DXGI_FORMAT(dst->_format);
    CopySource.PlacedFootprint.Footprint.Width = width;
    CopySource.PlacedFootprint.Footprint.Height = height;
    CopySource.PlacedFootprint.Footprint.Depth = 1;
    CopySource.PlacedFootprint.Footprint.RowPitch = width * Texture::GetComponentSize(dst->GetFormat());
    CopySource.SubresourceIndex = 0;

    if (dst->GetFormat() == TextureFormat::BC1) {
        CopySource.PlacedFootprint.Footprint.RowPitch = width * 2;
    } else if (dst->GetFormat() == TextureFormat::BC7) {
        CopySource.PlacedFootprint.Footprint.RowPitch = width * 4;
    }

    D3D12_TEXTURE_COPY_LOCATION CopyDest = {};
    CopyDest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    CopyDest.pResource = dst->_resource->Resource;
    CopyDest.SubresourceIndex = mip;

    _commandList->CopyTextureRegion(&CopyDest, 0, 0, 0, &CopySource, nullptr);
}

void CommandBuffer::CopyTextureFileToTexture(Texture::Ptr dst, Buffer::Ptr srcTexels, TextureFile *file)
{
    uint32_t numMips = file->MipCount();
    
    D3D12_RESOURCE_DESC desc = dst->GetResource().Resource->GetDesc();

    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(numMips);
    std::vector<uint32_t> numRows(numMips);
    std::vector<uint64_t> rowSizes(numMips);
    uint64_t totalSize = 0;

    _device->GetDevice()->GetCopyableFootprints(&desc, 0, numMips, 0, footprints.data(), numRows.data(), rowSizes.data(), &totalSize);

    for (uint32_t i = 0; i < numMips; i++) {
        D3D12_TEXTURE_COPY_LOCATION srcCopy = {};
        srcCopy.pResource = srcTexels->_resource->Resource;
        srcCopy.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcCopy.PlacedFootprint = footprints[i];

        D3D12_TEXTURE_COPY_LOCATION dstCopy = {};
        dstCopy.pResource = dst->_resource->Resource;
        dstCopy.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstCopy.SubresourceIndex = i;

        _commandList->CopyTextureRegion(&dstCopy, 0, 0, 0, &srcCopy, nullptr);
    }
}

void CommandBuffer::CopyTextureToBuffer(Buffer::Ptr dst, Texture::Ptr src)
{
    D3D12_TEXTURE_COPY_LOCATION CopySource = {};
    CopySource.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    CopySource.pResource = src->_resource->Resource;
    CopySource.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION CopyDest = {};
    CopyDest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    CopyDest.pResource = dst->_resource->Resource;
    CopyDest.PlacedFootprint.Offset = 0;
    CopyDest.PlacedFootprint.Footprint.Format = DXGI_FORMAT(src->_format);
    CopyDest.PlacedFootprint.Footprint.Width = src->_width;
    CopyDest.PlacedFootprint.Footprint.Height = src->_height;
    CopyDest.PlacedFootprint.Footprint.Depth = 1;
    CopyDest.PlacedFootprint.Footprint.RowPitch = src->_width * Texture::GetComponentSize(src->GetFormat());
    CopyDest.SubresourceIndex = 0;

    _commandList->CopyTextureRegion(&CopyDest, 0, 0, 0, &CopySource, nullptr);
}

void CommandBuffer::BuildAccelerationStructure(AccelerationStructure structure, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.DestAccelerationStructureData = structure.AS->Resource->GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = structure.Scratch->Resource->GetGPUVirtualAddress();

    _commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
}

void CommandBuffer::BeginImGui(int width, int height)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = width;
    io.DisplaySize.y = height;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void CommandBuffer::EndImGui()
{
    ImGuiIO& io = ImGui::GetIO();

    ID3D12DescriptorHeap* pHeaps[] = { _heaps.ShaderHeap->GetHeap(), _heaps.SamplerHeap->GetHeap() };
    _commandList->SetDescriptorHeaps(2, pHeaps);

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _commandList);
}

void CommandBuffer::CleanupImGui()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void*)_commandList);
    }
}
