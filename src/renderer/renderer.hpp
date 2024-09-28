/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:32:12
 */

#pragma once

#include "scene.hpp"
#include "core/timer.hpp"

#include "techniques/shadows.hpp"
#include "techniques/deferred.hpp"
#include "techniques/envmap_forward.hpp"

#include "techniques/temporal_anti_aliasing.hpp"
#include "techniques/motion_blur.hpp"
#include "techniques/chromatic_aberration.hpp"
#include "techniques/bloom.hpp"
#include "techniques/color_correction.hpp"
#include "techniques/auto_exposure.hpp"
#include "techniques/tonemapping.hpp"

#include "techniques/debug_renderer.hpp"

#include <vector>

class Renderer
{
public:
    struct Statistics
    {
        Timer timer;
        std::unordered_map<std::string, std::vector<float>> FrameTimesHistory;

        void PushFrameTime(const std::string& key, const std::function<void()>& function) {
            timer.Restart();
            float start = timer.GetElapsed();
            function();
            float end = timer.GetElapsed();
            
            FrameTimesHistory[key].push_back(end - start);
            if (FrameTimesHistory.size() > 100) {
                FrameTimesHistory.erase(FrameTimesHistory.begin());
            }
        }
    };

    Renderer(RenderContext::Ptr context);
    ~Renderer();
    
    void Render(Scene& scene, uint32_t width, uint32_t height, float dt);
    void Resize(uint32_t width, uint32_t height);
    void OnUI();
    void Screenshot(Texture::Ptr screenshotTexture = nullptr, TextureLayout newLayout = TextureLayout::ShaderResource);
    
    // Perform hot reloads
    void Reconstruct();

    Statistics& GetStats() { return _stats; }
private:
    Statistics _stats;
    RenderContext::Ptr _renderContext;

    // Geometry
    std::shared_ptr<Shadows> _shadows;
    std::shared_ptr<Deferred> _deferred;
    std::shared_ptr<EnvMapForward> _envMapForward;

    // Post process graph
    std::shared_ptr<TemporalAntiAliasing> _taa;
    std::shared_ptr<MotionBlur> _motionBlur;
    std::shared_ptr<ChromaticAberration> _chromaticAberration;
    std::shared_ptr<Bloom> _bloom;
    std::shared_ptr<ColorCorrection> _colorCorrection;
    std::shared_ptr<AutoExposure> _autoExposure;
    std::shared_ptr<Tonemapping> _tonemapping;
    
    // Debug renderer
    std::shared_ptr<DebugRenderer> _debugRenderer;
};
