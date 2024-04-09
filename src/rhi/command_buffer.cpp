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

bool IsHDR(TextureFormat format)
{
    if (format == TextureFormat::RGBA32Float)
        return true;
    return false;
}

CommandBuffer::CommandBuffer(Device::Ptr devicePtr, DescriptorHeap::Heaps& heaps, CommandQueueType type)
    : _type(D3D12_COMMAND_LIST_TYPE(type)), _heaps(heaps)
{
    HRESULT result = devicePtr->GetDevice()->CreateCommandAllocator(_type, IID_PPV_ARGS(&_commandAllocator));
    if (FAILED(result)) {
        Logger::Error("D3D12: Failed to create command allocator!");
    }

    result = devicePtr->GetDevice()->CreateCommandList(0, _type, _commandAllocator, nullptr, IID_PPV_ARGS(&_commandList));
    if (FAILED(result)) {
        Logger::Error("D3D12: Failed to create command list!");
    }

    result = _commandList->Close();
    if (FAILED(result)) {
        Logger::Error("D3D12: Failed to close freshly created command list!");
    }
}

CommandBuffer::~CommandBuffer()
{
    _commandList->Release();
    _commandAllocator->Release();
}

void CommandBuffer::Begin()
{
    _commandAllocator->Reset();
    _commandList->Reset(_commandAllocator, nullptr);

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

void CommandBuffer::ImageBarrier(Texture::Ptr texture, TextureLayout newLayout)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture->GetResource().Resource;
    barrier.Transition.StateBefore = texture->GetState();
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATES(newLayout);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    if (barrier.Transition.StateBefore == barrier.Transition.StateAfter)
        return;

    _commandList->ResourceBarrier(1, &barrier);

    texture->SetState(D3D12_RESOURCE_STATES(newLayout));
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

void CommandBuffer::ImageBarrier(Texture::Ptr texture, D3D12_RESOURCE_STATES state)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture->GetResource().Resource;
    barrier.Transition.StateBefore = texture->GetState();
    barrier.Transition.StateAfter = state;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    if (barrier.Transition.StateBefore == barrier.Transition.StateAfter)
        return;

    _commandList->ResourceBarrier(1, &barrier);

    texture->SetState(state);
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

void CommandBuffer::BindRenderTargets(const std::vector<Texture::Ptr> renderTargets, Texture::Ptr depthTarget)
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

void CommandBuffer::BindVertexBuffer(Buffer::Ptr buffer)
{
    _commandList->IASetVertexBuffers(0, 1, &buffer->_VBV);
}

void CommandBuffer::BindIndexBuffer(Buffer::Ptr buffer)
{
    _commandList->IASetIndexBuffer(&buffer->_IBV);
}

void CommandBuffer::BindGraphicsPipeline(GraphicsPipeline::Ptr pipeline)
{
    _commandList->SetPipelineState(pipeline->GetPipeline());
    _commandList->SetGraphicsRootSignature(pipeline->GetRootSignature());
}

void CommandBuffer::BindGraphicsConstantBuffer(Buffer::Ptr buffer, int index)
{
    _commandList->SetGraphicsRootDescriptorTable(index, buffer->_descriptor.GPU);
}

void CommandBuffer::BindGraphicsShaderResource(Texture::Ptr texture, int index)
{
    _commandList->SetGraphicsRootDescriptorTable(index, texture->_srv.GPU);
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
    _commandList->SetComputeRootSignature(pipeline->GetRootSignature());
}

void CommandBuffer::BindComputeShaderResource(Texture::Ptr texture, int index)
{
    _commandList->SetComputeRootDescriptorTable(index, texture->_srv.GPU);
}

void CommandBuffer::BindComputeStorageTexture(Texture::Ptr texture, int index)
{
    _commandList->SetComputeRootDescriptorTable(index, texture->_uav.GPU);
}

void CommandBuffer::BindComputeCubeMapShaderResource(CubeMap::Ptr texture, int index)
{
    _commandList->SetComputeRootDescriptorTable(index, texture->_srv.GPU);
}

void CommandBuffer::BindComputeCubeMapStorage(CubeMap::Ptr texture, int index)
{
    _commandList->SetComputeRootDescriptorTable(index, texture->_uav.GPU);
}

void CommandBuffer::BindComputeConstantBuffer(Buffer::Ptr buffer, int index)
{
    _commandList->SetComputeRootDescriptorTable(index, buffer->_descriptor.GPU);
}

void CommandBuffer::BindComputeSampler(Sampler::Ptr sampler, int index)
{
    _commandList->SetComputeRootDescriptorTable(index, sampler->GetDescriptor().GPU);
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

    D3D12_TEXTURE_COPY_LOCATION CopyDest = {};
    CopyDest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    CopyDest.pResource = dst->_resource->Resource;
    CopyDest.SubresourceIndex = 0;

    _commandList->CopyTextureRegion(&CopyDest, 0, 0, 0, &CopySource, nullptr);
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
