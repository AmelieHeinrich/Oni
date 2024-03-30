/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 20:05:25
 */

#pragma once

#include "core/log.hpp"
#include "core/window.hpp"

#include "rhi/render_context.hpp"

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

    GraphicsPipeline::Ptr _triPipeline;
    Buffer::Ptr _vertexBuffer;
    Buffer::Ptr _indexBuffer;
};
