//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 12:25:10
//

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

#include "renderer/hot_reloadable_pipeline.hpp"

#include <array>

class Bloom
{
public:
    Bloom(RenderContext::Ptr context, Texture::Ptr inputHDR);
    ~Bloom() {}

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR);
    void OnUI();
    void Reconstruct();

    void ConnectEmissiveBuffer(Texture::Ptr texture);
private:
    void Downsample(Scene& scene, uint32_t width, uint32_t height);
    void Upsample(Scene& scene, uint32_t width, uint32_t height);
    void Composite(Scene& scene, uint32_t width, uint32_t height);

    Texture::Ptr _emissionBuffer;
    Texture::Ptr _output;

    RenderContext::Ptr _context;
    float _filterRadius = 0.005f;
    float _bloomStrenght = 3.0f;

    bool _enable = true;
    int MIP_CAP = 8;

    HotReloadablePipeline _downsamplePipeline;
    HotReloadablePipeline _upsamplePipeline;
    HotReloadablePipeline _compositePipeline;

    Texture::Ptr _bloomFramebuffer;
    
    Sampler::Ptr _linearClamp;
    Sampler::Ptr _linearBorder;
    Sampler::Ptr _pointClamp;
};
