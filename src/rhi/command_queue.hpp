/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 19:58:41
 */

#pragma once

#include "Device.hpp"

enum class CommandQueueType
{
    Graphics = D3D12_COMMAND_LIST_TYPE_DIRECT,
    Compute = D3D12_COMMAND_LIST_TYPE_COMPUTE,
    Copy = D3D12_COMMAND_LIST_TYPE_COPY
};

class CommandQueue
{
public:
    using Ptr = std::shared_ptr<CommandQueue>;

    CommandQueue(Device::Ptr device, CommandQueueType type);
    ~CommandQueue();

    ID3D12CommandQueue* GetQueue() { return _queue; }
private:
    Device::Ptr _devicePtr;
    D3D12_COMMAND_LIST_TYPE _type;
    ID3D12CommandQueue *_queue;
};
