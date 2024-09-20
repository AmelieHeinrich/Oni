//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 12:25:10
//

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

#include "renderer/hot_reloadable_pipeline.hpp"

class Bloom
{
public:
    Bloom(RenderContext::Ptr context, Texture::Ptr inputHDR);
    ~Bloom() {}

    void Render(Scene& scene, uint32_t width, uint32_t height, float dt) {}
    void Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR) {}
    void OnUI() {}
    void Reconstruct() {}

private:
    RenderContext::Ptr _context;

    HotReloadablePipeline _downsamplePipeline;
};
