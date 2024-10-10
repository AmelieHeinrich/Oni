//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 17:44:40
//

#pragma once

#include <glm/glm.hpp>

#include "rhi/device.hpp"
#include "rhi/buffer.hpp"
#include "rhi/allocator.hpp"

#include "acceleration_structure.hpp"

class BLAS
{
public:
    using Ptr = std::shared_ptr<BLAS>;

    BLAS(Buffer::Ptr vertexBuffer, Buffer::Ptr indexBuffer, int vertexCount, int indexCount, const std::string& name = "BLAS");
    ~BLAS();

    int GetInstanceIndex() { return _blasInstanceIndex; }
private:
    friend class RenderContext;

    static int _instanceCounter;
    int _blasInstanceIndex = 0;

    AccelerationStructure _accelerationStructure = {};

    D3D12_RAYTRACING_GEOMETRY_DESC _geometryDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS _inputs = {};
};
