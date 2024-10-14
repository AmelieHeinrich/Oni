// forward_plus.hpp

#pragma once

#include "rhi/render_context.hpp"

#include "renderer/scene.hpp"
#include "renderer/hot_reloadable_pipeline.hpp"

#include "envmap_forward.hpp"

class ForwardPlus
{
public:
    ForwardPlus(RenderContext::Ptr renderContext);
    ~ForwardPlus() = default;

    void Render(Scene& scene, uint32_t width, uint32_t height, bool rtShadows);
    void Resize(uint32_t width, uint32_t height) {}
    void OnUI();
    void Reconstruct();

    void ConnectEnvironmentMap(EnvironmentMap& map) {}
    void ConnectShadowMap(Texture::Ptr texture) {}
    void ConnectSSAO(Texture::Ptr texture) { _ssao = texture; }

    void ShouldJitter(bool jitter) { _jitter = jitter; }

    Texture::Ptr GetOutput() { return _outputImage; }
    Texture::Ptr GetDepthBuffer() { return _depthBuffer; }
    Texture::Ptr GetVelocityBuffer() { return _velocityBuffer; }
    Texture::Ptr GetEmissiveBuffer() { return _emissive; }

    bool UseMeshShaders() { return _useMesh; }

    void ZPrepassMesh(Scene& scene, uint32_t width, uint32_t height) {}
    void ZPrepassClassic(Scene& scene, uint32_t width, uint32_t height) {}
    void LightCullPass(Scene& scene, uint32_t width, uint32_t height) {}
    void LightingMesh(Scene& scene, uint32_t width, uint32_t height, bool rtShadows) {}
    void LightingClassic(Scene& scene, uint32_t width, uint32_t height, bool rtShadows) {}

private:
    RenderContext::Ptr _context;
    EnvironmentMap _map;
    Texture::Ptr _shadowMap;

    Texture::Ptr _whiteTexture;
    Texture::Ptr _blackTexture;
    Texture::Ptr _outputImage;
    Texture::Ptr _depthBuffer;
    Texture::Ptr _velocityBuffer;
    Texture::Ptr _emissive;

    Texture::Ptr _ssao;

    HotReloadablePipeline _zprepassMesh;
    HotReloadablePipeline _zprepassClassic;

    std::array<glm::vec2, 16> _haltonSequence;
    glm::vec2 _currJitter;
    glm::vec2 _prevJitter;
    int _jitterCounter = 0;
    bool _jitter = true;

    bool _draw = true;
    bool _useMesh = true;
    bool _drawMeshlets = false;
};
