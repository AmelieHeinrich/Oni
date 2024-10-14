// forward_plus.cpp

#include "forward_plus.hpp"

ForwardPlus::ForwardPlus(RenderContext::Ptr context)
    : _zprepassMesh(PipelineType::Mesh), _zprepassClassic(PipelineType::Graphics), _context(context)
{
    uint32_t width, height;
    context->GetWindow()->GetSize(width, height);

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
        _zprepassMesh.Build(context);
    }

    // ZPrepassClassic
    {

    }

    // Light cull
    {

    }

    // LightingMesh
    {

    }

    // LightingClassic
    {

    }

    _velocityBuffer = _context->CreateTexture(width, height, TextureFormat::RG16Float, TextureUsage::RenderTarget, false, "[FORWARD+] Velocity buffer");
    _velocityBuffer->BuildRenderTarget();
    _velocityBuffer->BuildShaderResource();
    
    _emissive = _context->CreateTexture(width, height, TextureFormat::RGBA16Float, TextureUsage::RenderTarget, false, "[FORWARD+] Emissive");
    _emissive->BuildRenderTarget();
    _emissive->BuildShaderResource();

    _whiteTexture = context->CreateTexture(1, 1, TextureFormat::RGBA8, TextureUsage::ShaderResource, false, "[FORWARD+] White Texture");
    _whiteTexture->BuildShaderResource();

    _blackTexture = context->CreateTexture(1, 1, TextureFormat::RGBA8, TextureUsage::ShaderResource, false, "[FORWARD+] Black Texture");
    _blackTexture->BuildShaderResource();

    Uploader uploader = context->CreateUploader();

    uint32_t whiteColor = 0xFFFFFFFF;
    Bitmap whiteImage;
    whiteImage.Width = 1;
    whiteImage.Height = 1;
    whiteImage.Delete = false;
    whiteImage.Bytes = reinterpret_cast<char*>(&whiteColor);
    uploader.CopyHostToDeviceTexture(whiteImage, _whiteTexture);

    uint32_t blackColor = 0xFF000000;
    Bitmap blackImage;
    blackImage.Width = 1;
    blackImage.Height = 1;
    blackImage.Delete = false;
    blackImage.Bytes = reinterpret_cast<char*>(&blackColor);
    uploader.CopyHostToDeviceTexture(blackImage, _blackTexture);

    _context->FlushUploader(uploader);

    _outputImage = context->CreateTexture(width, height, TextureFormat::RGBA16Unorm, TextureUsage::RenderTarget, false, "[FORWARD+] Output");
    _outputImage->BuildRenderTarget();
    _outputImage->BuildShaderResource();
    _outputImage->BuildStorage();

    _depthBuffer = context->CreateTexture(width, height, TextureFormat::R32Typeless, TextureUsage::DepthTarget, false, "[FORWARD+] Depth Buffer");
    _depthBuffer->BuildDepthTarget(TextureFormat::R32Depth);
    _depthBuffer->BuildShaderResource(TextureFormat::R32Float);

    // Generate halton sequence
    _haltonSequence = {
        glm::vec2(0.500000f, 0.333333f),
        glm::vec2(0.250000f, 0.666667f),
        glm::vec2(0.750000f, 0.111111f),
        glm::vec2(0.125000f, 0.444444f),
        glm::vec2(0.625000f, 0.777778f),
        glm::vec2(0.375000f, 0.222222f),
        glm::vec2(0.875000f, 0.555556f),
        glm::vec2(0.062500f, 0.888889f),
        glm::vec2(0.562500f, 0.037037f),
        glm::vec2(0.312500f, 0.370370f),
        glm::vec2(0.812500f, 0.703704f),
        glm::vec2(0.187500f, 0.148148f),
        glm::vec2(0.687500f, 0.481481f),
        glm::vec2(0.437500f, 0.814815f),
        glm::vec2(0.937500f, 0.259259f),
        glm::vec2(0.031250f, 0.592593f)
    };
    for (int i = 0; i < _haltonSequence.size(); i++) {
        _haltonSequence[i].x = ((_haltonSequence[i].x - 0.5f) / width) * 2;
        _haltonSequence[i].y = ((_haltonSequence[i].y - 0.5f) / height) * 2;
    }
}

void ForwardPlus::Render(Scene& scene, uint32_t width, uint32_t height, bool rtShadows)
{
    if (_useMesh) {
        ZPrepassMesh(scene, width, height);
        LightCullPass(scene, width, height);
        LightingMesh(scene, width, height, rtShadows);
    } else {
        ZPrepassClassic(scene, width, height);
        LightCullPass(scene, width, height);
        LightingClassic(scene, width, height, rtShadows);
    }
}

void ForwardPlus::OnUI()
{
    if (ImGui::TreeNodeEx("Forward+", ImGuiTreeNodeFlags_Framed)) {
        // TODO
        ImGui::TreePop();
    }
}

void ForwardPlus::Reconstruct()
{
    if (_useMesh) {
        _zprepassMesh.CheckForRebuild(_context, "ZPrepass Mesh");
    }
}
