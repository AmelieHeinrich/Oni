//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 18:57:50
//

#include "blas.hpp"

BLAS::BLAS(Buffer::Ptr vertexBuffer, Buffer::Ptr indexBuffer, int vertexCount, int indexCount, const std::string& name)
{
    _geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    _geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    _geometryDesc.Triangles.IndexBuffer = indexBuffer->_resource->Resource->GetGPUVirtualAddress();
    _geometryDesc.Triangles.IndexCount = indexCount;
    _geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    _geometryDesc.Triangles.VertexCount = vertexCount;
    _geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    _geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->_resource->Resource->GetGPUVirtualAddress();
    _geometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexBuffer->_stride;
    _geometryDesc.Triangles.Transform3x4 = 0;

    _inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    _inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    _inputs.NumDescs = 1;
    _inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    _inputs.pGeometryDescs = &_geometryDesc;

    _accelerationStructure = ASBuilder::Allocate(_inputs, nullptr, name);
}

BLAS::~BLAS()
{
    _accelerationStructure.Scratch->Resource->Release();
    _accelerationStructure.Scratch->Allocation->Release();
    _accelerationStructure.Scratch->ClearFromAllocationList();

    _accelerationStructure.AS->Resource->Release();
    _accelerationStructure.AS->Allocation->Release();
    _accelerationStructure.AS->ClearFromAllocationList();
}
