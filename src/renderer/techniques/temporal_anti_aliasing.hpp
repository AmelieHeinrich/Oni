//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-27 17:20:12
//

#pragma once

#include "rhi/render_context.hpp"

#include "renderer/scene.hpp"
#include "renderer/hot_reloadable_pipeline.hpp"

class TemporalAntiAliasing
{
public:
    TemporalAntiAliasing(RenderContext::Ptr renderContext, Texture::Ptr output);
    ~TemporalAntiAliasing() = default;

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void OnUI();
    void Reconstruct();

    bool IsEnabled() { return _enabled; }

    void SetVelocityBuffer(Texture::Ptr texture) { _velocityBuffer = texture; }
private:
    RenderContext::Ptr _context;

    Texture::Ptr _velocityBuffer;
    Texture::Ptr _output;
    Texture::Ptr _history;

    bool _enabled = true;
};
