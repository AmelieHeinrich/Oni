/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:34:58
 */

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

class Deferred
{
public:
    Deferred(RenderContext::Ptr context);
    ~Deferred();

    void Render(Scene& scene);
    void Resize(uint32_t width, uint32_t height);
    void OnUI();

private:
    Texture::Ptr _OutputImage;
    Buffer::Ptr _quadBuffer;

    Texture::Ptr _gPosition;
    Texture::Ptr _gNormal;
    Texture::Ptr _gAlbedo;
    Texture::Ptr _gMetallicRoughness;

    GraphicsPipeline::Ptr _gBufferPipeline;
    GraphicsPipeline::Ptr _lightingPass;

    Buffer::Ptr _sceneBuffer;
    Buffer::Ptr _modelBuffer;
    Sampler::Ptr _sampler;
};
