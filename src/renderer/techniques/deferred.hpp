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

    void Render(Scene& scene, uint32_t width, uint32_t height, bool rtShadows);
    void Resize(uint32_t width, uint32_t height);
    void OnUI();
    void Reconstruct();

    void ConnectEnvironmentMap(EnvironmentMap& map);
    void ConnectShadowMap(Texture::Ptr texture);
    void ConnectSSAO(Texture::Ptr texture) { _ssao = texture; }

    void ShouldJitter(bool jitter) { _jitter = jitter; }

    Texture::Ptr GetOutput() { return _outputImage; }
    Texture::Ptr GetDepthBuffer() { return _depthBuffer; }
    Texture::Ptr GetNormalBuffer() { return _normals; }
    Texture::Ptr GetVelocityBuffer() { return _velocityBuffer; }
    Texture::Ptr GetEmissiveBuffer() { return _emissive; }

    bool UseMeshShaders() { return _useMesh; }

    void GBufferPassMesh(Scene& scene, uint32_t width, uint32_t height);
    void GBufferPassClassic(Scene& scene, uint32_t width, uint32_t height);
    void LightingPass(Scene& scene, uint32_t width, uint32_t height, bool rtShadows);
private:
    RenderContext::Ptr _context;
    EnvironmentMap _map;
    Texture::Ptr _shadowMap;

    // GBuffer
    Texture::Ptr _normals;
    Texture::Ptr _albedoEmission;
    Texture::Ptr _pbrData;
    Texture::Ptr _velocityBuffer;
    Texture::Ptr _emissive;

    Texture::Ptr _whiteTexture;
    Texture::Ptr _blackTexture;
    Texture::Ptr _outputImage;
    Texture::Ptr _depthBuffer;

    Texture::Ptr _ssao;

    HotReloadablePipeline _gbufferPipeline;
    HotReloadablePipeline _gbufferPipelineMesh;
    HotReloadablePipeline _lightingPipeline;

    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> _sceneBufferLight;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> _lightBuffer;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> _modeBuffer;

    Sampler::Ptr _anisotropicSampler;
    Sampler::Ptr _sampler;
    Sampler::Ptr _cubeSampler;
    Sampler::Ptr _shadowSampler;

    std::array<glm::vec2, 16> _haltonSequence;
    glm::vec2 _currJitter;
    glm::vec2 _prevJitter;
    int _jitterCounter = 0;
    bool _jitter = true;

    bool _draw = true;
    bool _useMesh = true;
    bool _drawMeshlets = false;

    int _mode = 0;
    bool _visualizeShadow = false;
    bool _pbr = true;
    float _directTerm = 1.0f;
    float _indirectTerm = 0.4f;
    float _emissiveStrength = 5.0f;
    bool _ibl = true;

    int _totalMeshes = 0;
    int _culledMeshes = 0;
};
