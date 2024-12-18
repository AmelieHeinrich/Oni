/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 17:22:47
 */

#include "sampler.hpp"

Sampler::Sampler(Device::Ptr device, DescriptorHeap::Heaps& heaps, SamplerAddress address, SamplerFilter filter, bool mips, int anisotropyLevel)
    : _heaps(heaps), _address(address), _filter(filter), _mips(mips), _anisotropyLevel(anisotropyLevel)
{
    D3D12_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE(address);
    SamplerDesc.AddressV = SamplerDesc.AddressU;
    SamplerDesc.AddressW = SamplerDesc.AddressV;
    SamplerDesc.Filter = D3D12_FILTER(filter);
    SamplerDesc.MaxAnisotropy = anisotropyLevel;
    SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    SamplerDesc.MinLOD = 0.0f;
    if (mips) {
        SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    }
    SamplerDesc.MipLODBias = 0.0f;
    SamplerDesc.BorderColor[0] = 1.0f;
    SamplerDesc.BorderColor[1] = 1.0f;
    SamplerDesc.BorderColor[2] = 1.0f;
    SamplerDesc.BorderColor[3] = 1.0f;

    _descriptor = heaps.SamplerHeap->Allocate();
    device->GetDevice()->CreateSampler(&SamplerDesc, _descriptor.CPU);
}

Sampler::~Sampler()
{
    _heaps.SamplerHeap->Free(_descriptor);
}
