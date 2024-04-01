/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 17:43:23
 */

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

class Forward
{
public:
    Forward(RenderContext::Ptr context);
    ~Forward();

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void OnUI();

    Texture::Ptr GetOutput() { return _outputImage; }
    Texture::Ptr GetDepthBuffer() { return _depthBuffer; }
private:
    RenderContext::Ptr _context;

    Texture::Ptr _whiteTexture;
    Texture::Ptr _outputImage;
    Texture::Ptr _depthBuffer;

    GraphicsPipeline::Ptr _forwardPipeline;

    Buffer::Ptr _sceneBuffer;
    Buffer::Ptr _modelBuffer;
    Buffer::Ptr _lightBuffer;
    Sampler::Ptr _sampler;
};
