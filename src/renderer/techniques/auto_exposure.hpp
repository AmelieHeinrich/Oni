/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-11 12:18:12
 */

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

#include "renderer/hot_reloadable_pipeline.hpp"

class AutoExposure
{
public:
    AutoExposure(RenderContext::Ptr context, Texture::Ptr inputHDR);
    ~AutoExposure();

    void Render(Scene& scene, uint32_t width, uint32_t height, float dt);
    void Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR);
    void OnUI();
    void Reconstruct();

    Texture::Ptr GetOutput() { return _inputHDR; }

private:
    RenderContext::Ptr _renderContext;

    HotReloadablePipeline _computePipeline;
    HotReloadablePipeline _averagePipeline;
    bool _enable = true;

    Texture::Ptr _inputHDR;
    Texture::Ptr _luminanceTexture;

    Buffer::Ptr _luminanceHistogram;
    Buffer::Ptr _luminanceHistogramParameters;
    Buffer::Ptr _averageParameters;

    float _minLogLuminance = -10.0f;
    float _luminanceRange = 12.0f;
    float _tau = 1.1f;
};
