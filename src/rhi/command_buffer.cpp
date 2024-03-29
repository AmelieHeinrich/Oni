/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-29 13:04:28
 */

#include "command_buffer.hpp"
#include "command_queue.hpp"

#include <core/log.hpp>

CommandBuffer::CommandBuffer(Device::Ptr devicePtr, CommandQueueType type)
    : _type(D3D12_COMMAND_LIST_TYPE(type))
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
