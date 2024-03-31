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

struct SceneBuffer
{
    glm::mat4 View;
    glm::mat4 Projection;
};

class App
{
public:
    App();
    ~App();

    void Run();

    void RenderOverlay();
private:
    std::unique_ptr<Window> _window;
    RenderContext::Ptr _renderContext;

    Timer _dtTimer;
    float _lastFrame;

    FreeCamera _camera;

    GraphicsPipeline::Ptr _triPipeline;
    Buffer::Ptr _sceneBuffer;
    Buffer::Ptr _modelBuffer;
    Texture::Ptr _depthBuffer;
    Sampler::Ptr _sampler;

    Model _model;
};
