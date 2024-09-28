//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-28 17:53:32
//

#pragma once

#include "rhi/render_context.hpp"

#include "renderer/scene.hpp"
#include "renderer/hot_reloadable_pipeline.hpp"

class MotionBlur
{
public:
    MotionBlur(RenderContext::Ptr context, Texture::Ptr output);
    ~MotionBlur() = default;

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void OnUI();
    void Reconstruct();

    void SetVelocityBuffer(Texture::Ptr texture) { _velocityBuffer = texture; }

private:
    RenderContext::Ptr _context;

    bool _enabled = false;
    uint32_t _sampleCount = 1;
    HotReloadablePipeline _blurPipeline;

    Texture::Ptr _velocityBuffer;
    Texture::Ptr _output;

    Sampler::Ptr _pointSampler;
};
