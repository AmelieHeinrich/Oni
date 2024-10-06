//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-15 00:04:03
//

#pragma once

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

#include "renderer/hot_reloadable_pipeline.hpp"
#include "core/model.hpp"

constexpr uint32_t MAX_LINES = 2048 * 2;

class DebugRenderer
{
public:
    struct Line {
        glm::vec3 A;
        glm::vec3 B;
        glm::vec3 Color;
    };

    struct AABBData {
        AABB BoundingBox;
        glm::mat4 Transform;
    };

    static void SetDebugRenderer(std::shared_ptr<DebugRenderer> renderer);
    static std::shared_ptr<DebugRenderer> Get();

    DebugRenderer(RenderContext::Ptr context, Texture::Ptr output);
    ~DebugRenderer() = default;

    void Resize(uint32_t width, uint32_t height, Texture::Ptr output);
    void OnUI();
    void Reconstruct();

    void PushLine(glm::vec3 a, glm::vec3 b, glm::vec3 color);
    void PushAABB(AABB aabb, glm::mat4 transform);
    
    void Flush(Scene& scene, uint32_t width, uint32_t height);
    void Reset();
    
    void SetVelocityBuffer(Texture::Ptr texture);

    Texture::Ptr GetOutput();
private:
    glm::vec4 ApplyTransform(glm::vec3 vector, glm::mat4 transform) {
        return glm::vec4(vector, 1.0) * transform;
    }

    RenderContext::Ptr _context;
    bool DrawLines = true;
    bool DrawMotion = false;
    bool DrawAABB = false;

    struct DrawList {
        std::vector<DebugRenderer::Line> Lines;
        std::vector<AABBData> BoundingBoxes;
    };

    struct LineVertex {
        glm::vec4 Vertex;
        glm::vec4 Color;
    };

    struct CubeVertex {
        glm::vec4 Vertex;
    };

    RenderContext::Ptr Context;
    Texture::Ptr Output;
    Texture::Ptr VelocityBuffer;

    DrawList List;

    HotReloadablePipeline MotionShader;
    Sampler::Ptr NearestSampler;

    HotReloadablePipeline LineShader;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> LineTransferBuffer;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> LineVertexBuffer;

    HotReloadablePipeline AABBShader;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> AABBTransferBuffer;
    std::array<Buffer::Ptr, FRAMES_IN_FLIGHT> AABBVertexBuffer;

    float LineWidth = 2.0f;
};
