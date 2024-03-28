/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:45:16
 */

#include "allocator.hpp"

#include <core/log.hpp>

Allocator::Allocator(Device::Ptr devicePtr)
{
    D3D12MA::ALLOCATOR_DESC desc = {};
    desc.pAdapter = devicePtr->GetAdapter();
    desc.pDevice = devicePtr->GetDevice();

    HRESULT result = D3D12MA::CreateAllocator(&desc, &_allocator);
    if (FAILED(result)) {
        Logger::Error("D3D12: Failed to create memory allocator!");
    }

    Logger::Info("D3D12: Successfully created memory allocator");
}

Allocator::~Allocator()
{
    _allocator->Release();
}
