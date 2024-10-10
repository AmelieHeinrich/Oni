//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 21:36:11
//

#include "rt_shadows.hpp"

RTShadows::RTShadows(RenderContext::Ptr context)
    : _context(context), _rtPipeline(PipelineType::Raytracing)
{
    _rtPipeline.RTSpecs.PayloadSize = 4 * sizeof(uint32_t);

    _rtPipeline.SignatureInfo = {
        { RootSignatureEntry::PushConstants },
        4 * sizeof(uint32_t)
    };
    _rtPipeline.ReflectRootSignature(false);
    _rtPipeline.AddShaderWatch("shaders/Raytracing/Shadows/RTShadowsLib.hlsl", "", ShaderType::Raytracing);
    _rtPipeline.Build(context);
}
