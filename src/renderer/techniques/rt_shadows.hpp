//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 21:30:55
//

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

#include "renderer/hot_reloadable_pipeline.hpp"

class RTShadows
{
public:
    RTShadows(RenderContext::Ptr context);
    ~RTShadows() = default;

    void Render(Scene& scene, uint32_t width, uint32_t height) {}
    void Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR) {}
    void OnUI() {}
    void Reconstruct() {}

private:
    RenderContext::Ptr _context;

    HotReloadablePipeline _rtPipeline;
};
