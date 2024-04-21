/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-11 13:41:11
 */

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

class ShadowMap
{
public:
    ShadowMap(RenderContext::Ptr context);
    ~ShadowMap();

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void OnUI();

    Texture::Ptr GetDepthBuffer() { return _depthBuffer; }

private:
    Texture::Ptr _depthBuffer;

    GraphicsPipeline::Ptr _shadowPipeline;
};
