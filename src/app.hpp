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
private:
    std::shared_ptr<Window> _window;

    RenderContext::Ptr _renderContext;
    std::unique_ptr<Renderer> _renderer;

    Timer _dtTimer;
    float _lastFrame;

    FreeCamera _camera;
    Scene scene;

    bool _showUI = false;
    bool _showMemoryUI = false;
};
