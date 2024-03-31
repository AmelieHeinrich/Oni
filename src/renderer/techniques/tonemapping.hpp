/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:45:38
 */

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

class Tonemapping
{
public:
    Tonemapping(RenderContext::Ptr context, Texture::Ptr inputHDR);
    ~Tonemapping();

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR);
    void OnUI();

    Texture::Ptr GetOutput() { return _outputLDR; }
private:
    RenderContext::Ptr _renderContext;

    ComputePipeline::Ptr _computePipeline;

    Texture::Ptr _inputHDR;
    Texture::Ptr _outputLDR;
};
