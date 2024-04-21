/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-09 23:10:22
 */

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

class ColorCorrection
{
public:
    ColorCorrection(RenderContext::Ptr context, Texture::Ptr inputHDR);
    ~ColorCorrection();

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR);
    void OnUI();

    Texture::Ptr GetOutput() { return _inputHDR; }
private:
    struct ColorCorrectionSettings
    {
        float Exposure = 2.2f;
        float Temperature = 0.0f;
        float Tint = 0.0f;
        uint32_t _Padding4;
        glm::vec3 Contrast = glm::vec3(1.0f);
        uint32_t _Padding5;
        glm::vec3 LinearMidPoint = glm::vec3(0.0f);
        uint32_t _Padding6;
        glm::vec3 Brightness = glm::vec3(0.0f);
        uint32_t _Padding7;
        glm::vec3 ColorFilter = glm::vec3(1.0f);
        uint32_t _Padding8;
        float ColorFilterIntensity = 1.0f;
        glm::vec3 Saturation = glm::vec3(1.0f);
    };

    RenderContext::Ptr _renderContext;

    ComputePipeline::Ptr _computePipeline;
    bool _enable = false;

    Texture::Ptr _inputHDR;
    Buffer::Ptr _correctionParameters;
    ColorCorrectionSettings _settings;
};
