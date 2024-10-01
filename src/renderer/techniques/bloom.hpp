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

    int MIP_COUNT = 8;

    struct BloomMip
    {
        glm::vec2 Size;
        glm::ivec2 IntSize;
        Texture::Ptr RenderTarget;
    };

    RenderContext::Ptr _context;
    float _filterRadius = 0.005f;
    float _bloomStrenght = 0.30f;

    bool _enable = true;

    HotReloadablePipeline _downsamplePipeline;
    HotReloadablePipeline _upsamplePipeline;
    HotReloadablePipeline _compositePipeline;

    std::vector<BloomMip> _mipChain;
    
    Sampler::Ptr _linearClamp;
    Sampler::Ptr _linearBorder;
    Sampler::Ptr _pointClamp;
};
