//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-07 19:12:20
//

#pragma once

#include "graphics_pipeline.hpp"

class MeshPipeline
{
public:
    using Ptr = std::shared_ptr<MeshPipeline>;

    MeshPipeline(Device::Ptr devicePtr, GraphicsPipelineSpecs& specs);
    ~MeshPipeline();

    ID3D12PipelineState* GetPipeline() { return _pipeline; }
    RootSignature::Ptr GetSignature() { return _signature; }
private:
    ID3D12PipelineState* _pipeline;
    RootSignature::Ptr _signature;
};
