/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:05:25
 */

#pragma once

#include "core/log.hpp"
#include "core/window.hpp"

#include "model.hpp"

#include "camera.hpp"
#include "timer.hpp"

#include "rhi/render_context.hpp"

#include "renderer/renderer.hpp"

class App
{
public:
    App();
    ~App();

    void Run();

    void RenderOverlay();
    void RenderHelper();
private:
    void ShowLightEditor();

    std::shared_ptr<Window> _window;

    RenderContext::Ptr _renderContext;
    std::unique_ptr<Renderer> _renderer;

    Timer _dtTimer;
    Timer _updateTimer;
    float _lastFrame;

    FreeCamera _camera;
    Scene scene;

    bool _showUI = false;
    bool _showResourceInspector = false;
    bool _showRendererSettings = false;
    bool _showLightEditor = false;
    int _fps;
    float _frameTime = 0.0f;
};
