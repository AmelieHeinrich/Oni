/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:43:16
 */

#pragma once

#include "device.hpp"

#include "D3D12MA/D3D12MemAlloc.h"

class Allocator
{
public:
    using Ptr = std::shared_ptr<Allocator>;

    Allocator(Device::Ptr devicePtr);
    ~Allocator();
private:
    D3D12MA::Allocator* _allocator;
};
