/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 17:17:34
 */

#pragma once

#include "device.hpp"
#include "descriptor_heap.hpp"

enum class SamplerAddress
{
    Wrap = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    Mirror = D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
    Clamp = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    Border = D3D12_TEXTURE_ADDRESS_MODE_BORDER
};

enum class SamplerFilter
{
    Linear = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    Nearest = D3D12_FILTER_MIN_MAG_MIP_POINT,
    Anistropic = D3D12_FILTER_ANISOTROPIC
};

class Sampler
{
public:
    using Ptr = std::shared_ptr<Sampler>;

    Sampler(Device::Ptr device, DescriptorHeap::Heaps& heaps, SamplerAddress address, SamplerFilter filter, int anisotropyLevel);
    ~Sampler();

    DescriptorHeap::Descriptor GetDescriptor() { return _descriptor; }
private:
    DescriptorHeap::Descriptor _descriptor;
    DescriptorHeap::Heaps _heaps;
};
