/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-09 23:10:22
 */

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

#include "renderer/hot_reloadable_pipeline.hpp"

class ColorCorrection
{
public:
    ColorCorrection(RenderContext::Ptr context, Texture::Ptr inputHDR);
    ~ColorCorrection();

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR);
    void OnUI();
    void Reconstruct();

    Texture::Ptr GetOutput() { return _inputHDR; }
private:
    // TODO(ame): Channel mixer + color picker for shadows/highlights/midtones
    // TODO(ame): LUT?
    struct ColorCorrectionSettings
    {
        float Exposure = 0.0f;
        glm::vec3 _Pad0;
        float Contrast = 0.0f;
        glm::vec3 _Pad1;
        glm::vec4 ColorFilter = glm::vec4(1.0f);
        float HueShift = 0.0f;
        float Saturation = 0.0f;
        float Temperature = 0.0f;
        float Tint = 0.0f;
        glm::vec4 Shadows = glm::vec4(0.4f);
        glm::vec4 Highlights = glm::vec4(0.4f);
        float Balance = 0.0f;
        glm::vec3 _Pad2;
    };

    RenderContext::Ptr _renderContext;

    HotReloadablePipeline _computePipeline;
    bool _enable = false;

    Texture::Ptr _inputHDR;
    Buffer::Ptr _correctionParameters;
    ColorCorrectionSettings _settings;
};
