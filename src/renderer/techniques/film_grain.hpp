//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-28 22:10:00
//

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

#include "renderer/hot_reloadable_pipeline.hpp"

class FilmGrain
{
public:
    FilmGrain(RenderContext::Ptr context, Texture::Ptr output);
    ~FilmGrain() = default;

    void Render(Scene& scene, uint32_t width, uint32_t height, float dt);
    void Resize(uint32_t width, uint32_t height, Texture::Ptr output);
    void OnUI();
    void Reconstruct();

private:
    RenderContext::Ptr _renderContext;

    HotReloadablePipeline _computePipeline;
    bool _enable = false;

    Texture::Ptr _inputHDR;
    float _amount = 0.1f;
};
