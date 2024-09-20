//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 12:35:27
//

#include "bloom.hpp"

Bloom::Bloom(RenderContext::Ptr context, Texture::Ptr inputHDR)
    : _context(context), _downsamplePipeline(PipelineType::Compute)
{
    _downsamplePipeline.AddShaderWatch("shaders/Bloom/Downsample/DownsampleCompute.hlsl", "Main", ShaderType::Compute);
    _downsamplePipeline.Build(context);
}
