/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:05:25
 */

#pragma once

#include "core/log.hpp"
#include "core/window.hpp"
#include "core/model.hpp"
#include "core/camera.hpp"
#include "core/timer.hpp"
#include "core/file_watch.hpp"

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

    void SetupScene();

    std::shared_ptr<Window> _window;

    RenderContext::Ptr _renderContext;
    std::unique_ptr<Renderer> _renderer;

    Timer _dtTimer;
    Timer _updateTimer;
    Timer _frameTimer;
    float _lastFrame;

    FreeCamera _camera;
    Scene scene;

    bool _vsync = false;
    bool _fpsAsGraph = false;
    bool _hideOverlay = false;
    bool _drawGrid = false;

    bool _showUI = false;
    bool _showResourceInspector = false;
    bool _showRendererSettings = false;
    bool _showLightEditor = false;
    bool _showLogger = false;
    bool _updateFrustum = true;

    std::vector<float> _pastFps;
    int _historySize = 500;

    int _fps;
    int _frameCount = 0;
    float _frameTime = 0.0f;
};
