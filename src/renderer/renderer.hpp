/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:32:12
 */

#pragma once

#include "scene.hpp"

#include "techniques/envmap_forward.hpp"
#include "techniques/forward.hpp"
#include "techniques/tonemapping.hpp"

class Renderer
{
public:
    Renderer(RenderContext::Ptr context);
    ~Renderer();
    
    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void OnUI();

private:
    RenderContext::Ptr _renderContext;

    std::shared_ptr<EnvMapForward> _envMapForward;
    std::shared_ptr<Forward> _forward;
    std::shared_ptr<Tonemapping> _tonemapping;
};
