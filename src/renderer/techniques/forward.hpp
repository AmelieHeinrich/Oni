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

    Texture::Ptr GetOutput() { return _outputImage; }
    Texture::Ptr GetDepthBuffer() { return _depthBuffer; }
private:
    RenderContext::Ptr _context;
    EnvironmentMap _map;

    Texture::Ptr _whiteTexture;
    Texture::Ptr _outputImage;
    Texture::Ptr _depthBuffer;

    GraphicsPipeline::Ptr _forwardPipeline;

    Buffer::Ptr _sceneBuffer;
    Buffer::Ptr _modelBuffer;
    Buffer::Ptr _lightBuffer;
    Buffer::Ptr _modeBuffer;
    Sampler::Ptr _sampler;

    int _mode = 0;
};
