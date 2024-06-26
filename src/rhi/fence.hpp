/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:18:03
 */

#pragma once

#include "command_queue.hpp"

class Fence
{
public:
    using Ptr = std::shared_ptr<Fence>;

    Fence(Device::Ptr device);
    ~Fence();

    uint64_t Signal(CommandQueue::Ptr _queue);
    void Wait(uint64_t target, uint64_t timeout);

    ID3D12Fence* GetFence() { return _fence; }
    
    uint64_t Value() { return _value; }
    uint64_t CompletedValue() { return _fence->GetCompletedValue(); }
private:
    Device::Ptr _devicePtr;
    uint64_t _value;
    ID3D12Fence* _fence;
};
