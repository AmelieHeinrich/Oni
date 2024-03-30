/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:05:25
 */

#pragma once

#include "core/log.hpp"
#include "core/window.hpp"

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
    std::unique_ptr<RenderContext> _renderContext;

    Timer _dtTimer;
    float _lastFrame;

    FreeCamera _camera;

    GraphicsPipeline::Ptr _triPipeline;
    Buffer::Ptr _vertexBuffer;
    Buffer::Ptr _indexBuffer;
    Buffer::Ptr _constantBuffer;
    Texture::Ptr _texture;
    Sampler::Ptr _sampler;
};
