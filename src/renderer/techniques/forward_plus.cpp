// forward_plus.cpp

#include "forward_plus.hpp"

ForwardPlus::ForwardPlus(RenderContext::Ptr renderContext)
    : _zprepassMesh(PipelineType::Mesh), _zprepassClassic(PipelineType::Graphics), _context(renderContext)
{
    // ZPrepassMesh
    {
        _zprepassMesh.Specs.FormatCount = 1;
        _zprepassMesh.Specs.Formats[0] = TextureFormat::RG16Float;
        _zprepassMesh.Specs.DepthFormat = TextureFormat::R32Depth;
        _zprepassMesh.Specs.Depth = DepthOperation::Less;
        _zprepassMesh.Specs.DepthEnabled = true;
        _zprepassMesh.Specs.Cull = CullMode::Front;
        _zprepassMesh.Specs.Fill = FillMode::Solid;
        _zprepassMesh.Specs.CCW = false;
        _zprepassMesh.Specs.UseAmplification = true;

        _zprepassMesh.SignatureInfo = {
            { RootSignatureEntry::PushConstants },
            9 * sizeof(uint32_t)
        };
        _zprepassMesh.ReflectRootSignature(false);
        _zprepassMesh.AddShaderWatch("shaders/Forward+/MS/ZPrepassAmplification.hlsl", "Main", ShaderType::Amplification);
        _zprepassMesh.AddShaderWatch("shaders/Forward+/MS/ZPrepassMesh.hlsl", "Main", ShaderType::Mesh);
        _zprepassMesh.AddShaderWatch("shaders/Forward+/MS/ZPrepassFrag.hlsl", "Main", ShaderType::Fragment);
        _zprepassMesh.Build(renderContext);
    }

    // ZPrepassClassic
    {

    }
}
