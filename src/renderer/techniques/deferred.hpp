//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-18 16:09:25
//

#pragma once

#include "rhi/render_context.hpp"

#include "renderer/scene.hpp"
#include "renderer/hot_reloadable_pipeline.hpp"

#include "envmap_forward.hpp"

class Deferred
{
public:
    Deferred(RenderContext::Ptr renderContext);
    ~Deferred();

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void OnUI();
    void Reconstruct();

    void ConnectEnvironmentMap(EnvironmentMap& map);
    void ConnectShadowMap(Texture::Ptr texture);

    Texture::Ptr GetOutput() { return _outputImage; }
    Texture::Ptr GetDepthBuffer() { return _depthBuffer; }
    Texture::Ptr GetVelocityBuffer() { return _velocityBuffer; }
private:
    void GBufferPass(Scene& scene, uint32_t width, uint32_t height);
    void LightingPass(Scene& scene, uint32_t width, uint32_t height);

    RenderContext::Ptr _context;
    EnvironmentMap _map;
    Texture::Ptr _shadowMap;

    // GBuffer
    Texture::Ptr _normals;
    Texture::Ptr _albedoEmission;
    Texture::Ptr _pbrData;
    Texture::Ptr _velocityBuffer;

    Texture::Ptr _whiteTexture;
    Texture::Ptr _blackTexture;
    Texture::Ptr _outputImage;
    Texture::Ptr _depthBuffer;

    HotReloadablePipeline _gbufferPipeline;
    HotReloadablePipeline _lightingPipeline;

    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> _sceneBufferLight;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> _lightBuffer;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> _modeBuffer;

    Sampler::Ptr _sampler;
    Sampler::Ptr _shadowSampler;

    int _mode = 0;
    bool _visualizeShadow = false;
    bool _pbr = true;
    bool _ibl = true;
    bool _draw = true;
};
