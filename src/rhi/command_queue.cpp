/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:02:03
 */

#include "command_queue.hpp"
#include "fence.hpp"

#include <core/log.hpp>

CommandQueue::CommandQueue(Device::Ptr device, CommandQueueType type)
    : _devicePtr(device), _type(D3D12_COMMAND_LIST_TYPE(type))
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = _type;

    HRESULT result = device->GetDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&_queue));
    if (FAILED(result)) {
        Logger::Error("Failed to create command queue!");
    }
}

CommandQueue::~CommandQueue()
{
    _queue->Release();
}

void CommandQueue::Wait(Fence::Ptr fence, uint64_t value)
{
    _queue->Wait(fence->GetFence(), value);
}

void CommandQueue::Submit(const std::vector<CommandBuffer::Ptr>& buffers)
{
    std::vector<ID3D12CommandList*> lists;
    for (auto& buffer : buffers) {
        lists.push_back(buffer->GetCommandList());
    }

    _queue->ExecuteCommandLists(lists.size(), lists.data());
}
