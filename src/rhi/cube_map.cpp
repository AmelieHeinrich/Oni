/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 00:47:16
 */

#include "cube_map.hpp"

CubeMap::CubeMap(Device::Ptr devicePtr, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps, uint32_t width, uint32_t height, TextureFormat format)
    : _devicePtr(devicePtr), _heaps(heaps), _format(format), _width(width), _height(height)
{
    D3D12MA::ALLOCATION_DESC AllocationDesc = {};
    AllocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    ResourceDesc.Width = width;
    ResourceDesc.Height = height;
    ResourceDesc.DepthOrArraySize = 6;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT(format);
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    _state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    _resource = allocator->Allocate(&AllocationDesc, &ResourceDesc, _state);

    _srv = _heaps.ShaderHeap->Allocate();
    _uav = _heaps.ShaderHeap->Allocate();
    
    D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceView = {};
    ShaderResourceView.Format = DXGI_FORMAT(_format);
    ShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    ShaderResourceView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    ShaderResourceView.TextureCube.MipLevels = 1;

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAV = {};
    UAV.Format = DXGI_FORMAT(_format);
    UAV.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    UAV.Texture2DArray.ArraySize = 6;
    UAV.Texture2DArray.FirstArraySlice = 0;
    UAV.Texture2DArray.MipSlice = 0;

    _devicePtr->GetDevice()->CreateShaderResourceView(_resource.Resource, &ShaderResourceView, _srv.CPU);
    _devicePtr->GetDevice()->CreateUnorderedAccessView(_resource.Resource, nullptr, &UAV, _uav.CPU);
}

CubeMap::~CubeMap()
{
    _heaps.ShaderHeap->Free(_srv);
    _heaps.ShaderHeap->Free(_uav);
    _resource.Allocation->Release();
}