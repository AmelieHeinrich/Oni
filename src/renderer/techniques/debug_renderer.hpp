//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-15 00:04:03
//

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

constexpr uint32_t MAX_LINES = 2048 * 2;

class DebugRenderer
{
public:
    struct Line {
        glm::vec3 A;
        glm::vec3 B;
        glm::vec3 Color;
    };

    static void SetDebugRenderer(std::shared_ptr<DebugRenderer> renderer);
    static std::shared_ptr<DebugRenderer> Get();

    DebugRenderer(RenderContext::Ptr context, Texture::Ptr output);
    ~DebugRenderer() = default;

    void Resize(uint32_t width, uint32_t height, Texture::Ptr output);
    void OnUI();
    
    void PushLine(glm::vec3 a, glm::vec3 b, glm::vec3 color);
    
    void Flush(Scene& scene, uint32_t width, uint32_t height);
    void Reset();
    
    Texture::Ptr GetOutput();
private:
    bool DrawLines = true;

    struct DrawList {
        std::vector<DebugRenderer::Line> Lines;
    };

    struct LineVertex {
        glm::vec4 Vertex;
        glm::vec4 Color;
    };

    RenderContext::Ptr Context;
    Texture::Ptr Output;

    DrawList List;

    GraphicsPipeline::Ptr LineShader;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> LineTransferBuffer;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> LineVertexBuffer;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> LineUniformBuffer;

    float LineWidth = 2.0f;
};
