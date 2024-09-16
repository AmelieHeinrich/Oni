/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 17:43:23
 */

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

#include "envmap_forward.hpp"

#define FORWARD_MODE_DEFAULT 0
#define FORWARD_MODE_ALBEDO 1
#define FORWARD_MODE_NORMAL 2
#define FORWARD_MODE_MR 3
#define FORWARD_MODE_AO 4
#define FORWARD_MODE_EMISSIVE 5
#define FORWARD_MODE_SPECULAR 6
#define FORWARD_MODE_AMBIENT 7

class Forward
{
public:
    Forward(RenderContext::Ptr context);
    ~Forward();

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void OnUI();
    void ConnectEnvironmentMap(EnvironmentMap& map);
    void ConnectShadowMap(Texture::Ptr texture);

    Texture::Ptr GetOutput() { return _outputImage; }
    Texture::Ptr GetDepthBuffer() { return _depthBuffer; }
private:
    void RenderPBR(Scene& scene, uint32_t width, uint32_t height);

    RenderContext::Ptr _context;
    EnvironmentMap _map;

    Texture::Ptr _whiteTexture;
    Texture::Ptr _outputImage;
    Texture::Ptr _depthBuffer;
    Texture::Ptr _shadowMap;

    GraphicsPipeline::Ptr _pbrPipeline;

    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> _sceneBuffer;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> _lightBuffer;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> _modeBuffer;

    Sampler::Ptr _sampler;
    Sampler::Ptr _shadowSampler;

    int _mode = 0;
    bool _visualizeShadow = false;
    bool _pbr = true;
    bool _ibl = true;
    bool _draw = true;

    bool _reconstruct = false;
};
