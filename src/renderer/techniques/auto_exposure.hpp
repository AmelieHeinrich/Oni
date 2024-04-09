/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-06 18:33:44
 */

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

class AutoExposure
{
public:
    AutoExposure(Texture::Ptr hdrTexture);
    ~AutoExposure();

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height, Texture::Ptr inputColor);
    void OnUI();

private:
    ComputePipeline::Ptr _luminancePipeline;
    ComputePipeline::Ptr _averagePipeline;
    Buffer::Ptr _luminanceParams;
    Buffer::Ptr _averageParams;
    Buffer::Ptr _luminanceHistogram;
    Texture::Ptr _luminanceTexture;
};
