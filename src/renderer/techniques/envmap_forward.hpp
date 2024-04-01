/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 01:11:48
 */

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

struct EnvironmentMap
{
    CubeMap::Ptr Environment;
    CubeMap::Ptr PrefilterMap;
    CubeMap::Ptr IrradianceMap;
    Texture::Ptr BRDF;
};

class EnvMapForward
{
public:
    EnvMapForward(RenderContext::Ptr context, Texture::Ptr inputColor, Texture::Ptr inputDepth);
    ~EnvMapForward();

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height, Texture::Ptr inputColor, Texture::Ptr inputDepth);
    void OnUI();

    EnvironmentMap GetEnvMap() { return _map; }
private:
    RenderContext::Ptr _context;
    Texture::Ptr _inputColor;
    Texture::Ptr _inputDepth;

    EnvironmentMap _map;

    Sampler::Ptr _cubeSampler;

    ComputePipeline::Ptr _envToCube;
    ComputePipeline::Ptr _prefilter;
    ComputePipeline::Ptr _irradiance;
    ComputePipeline::Ptr _brdf;
    GraphicsPipeline::Ptr _cubeRenderer;

    Buffer::Ptr _cubeBuffer;
    Buffer::Ptr _cubeCBV;
};
