/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 17:22:47
 */

#include "sampler.hpp"

Sampler::Sampler(Device::Ptr device, DescriptorHeap::Heaps& heaps, SamplerAddress address, SamplerFilter filter, int anisotropyLevel)
    : _heaps(heaps)
{
    D3D12_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE(address);
    SamplerDesc.AddressV = SamplerDesc.AddressU;
    SamplerDesc.AddressW = SamplerDesc.AddressV;
    SamplerDesc.Filter = D3D12_FILTER(filter);
    SamplerDesc.MaxAnisotropy = anisotropyLevel;
    SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    _descriptor = heaps.SamplerHeap->Allocate();
    device->GetDevice()->CreateSampler(&SamplerDesc, _descriptor.CPU);
}

Sampler::~Sampler()
{
    _heaps.SamplerHeap->Free(_descriptor);
}
