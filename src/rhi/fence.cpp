/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:19:33
 */

#include "fence.hpp"

#include <core/log.hpp>

Fence::Fence(Device::Ptr device)
    : _devicePtr(device), _value(0)
{
    HRESULT result = device->GetDevice()->CreateFence(_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
    if (FAILED(result)) {
        Logger::Error("Failed to create fence!");
    }
}

Fence::~Fence()
{
    _fence->Release();
}

uint64_t Fence::Signal(CommandQueue::Ptr _queue)
{
    _value++;
    _queue->GetQueue()->Signal(_fence, _value);
    return _value;
}

void Fence::Wait(uint64_t target, uint64_t timeout)
{
    if (_fence->GetCompletedValue() < target) {
        HANDLE event = CreateEvent(nullptr, false, false, nullptr);
        _fence->SetEventOnCompletion(target, event);
        if (WaitForSingleObject(event, timeout) == WAIT_TIMEOUT) {
            Logger::Error("!! GPU TIMEOUT !!");
        }
    }
}
