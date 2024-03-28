/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:02:03
 */

#include "command_queue.hpp"

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
