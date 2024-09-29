/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:45:38
 */

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

#include "renderer/hot_reloadable_pipeline.hpp"

class Tonemapping
{
public:
    Tonemapping(RenderContext::Ptr context, Texture::Ptr inputHDR);
    ~Tonemapping();

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR);
    void OnUI();
    void Reconstruct();

    Texture::Ptr GetOutput() { return _outputLDR; }
private:
    RenderContext::Ptr _renderContext;

    HotReloadablePipeline _computePipeline;

    Texture::Ptr _inputHDR;
    Texture::Ptr _outputLDR;

    uint32_t _tonemapper = 0;
    float _gamma = 2.2f;
};
