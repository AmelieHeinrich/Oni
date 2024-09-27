/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-07-15 00:47:17
 */

#pragma once

#include <array>

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>
#include <renderer/hot_reloadable_pipeline.hpp>

// Possible resolution :
//  - Very Low: 256x256
//  - Low: 512x512
//  - Medium: 1024x1024
//  - High: 2048x2048
//  - Ultra: 4096x4096

enum class ShadowMapResolution
{
    VeryLow = 256,
    Low = 512,
    Medium = 1024,
    High = 2048,
    Ultra = 4096
};

class Shadows
{
public:
    Shadows(RenderContext::Ptr context, ShadowMapResolution resolution);
    ~Shadows();

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height) {};
    void OnUI();
    void Reconstruct();

    Texture::Ptr GetOutput() { return _shadowMap; }

private:
    RenderContext::Ptr _context;

    HotReloadablePipeline _shadowPipeline;

    ShadowMapResolution _shadowMapResolution;
    Texture::Ptr _shadowMap;
    
    bool _renderShadows = true;
};
