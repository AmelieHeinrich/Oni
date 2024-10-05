//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-05 11:31:27
//

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

#include "renderer/hot_reloadable_pipeline.hpp"

class SSAO
{
public:
    SSAO(RenderContext::Ptr renderContext);
    ~SSAO() = default;

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void OnUI();
    void Reconstruct();

    Texture::Ptr GetOutput() { return _ssao; }

    void SetDepthBuffer(Texture::Ptr tex) { _depth = tex; }
    void SetNormalBuffer(Texture::Ptr tex) { _normals = tex; }
private:
    void SSAOPass(Scene& scene, uint32_t width, uint32_t height);
    void BlurPass(Scene& scene, uint32_t width, uint32_t height);

    RenderContext::Ptr _context;

    HotReloadablePipeline _ssaoPipeline;
    HotReloadablePipeline _ssaoBlur;

    Texture::Ptr _ssao;
    Texture::Ptr _noise;

    Texture::Ptr _depth;
    Texture::Ptr _normals;

    Buffer::Ptr _kernelBuffer;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> _cameraBuffer;
    
    Sampler::Ptr _pointSampler;
    Sampler::Ptr _pointClampSampler;

    // Settings
    uint32_t _kernelSize = 64;
    float _radius = 0.5;
    float _bias = 0.025;
    uint32_t _blurSize = 2;
    uint32_t _power = 5;
};
