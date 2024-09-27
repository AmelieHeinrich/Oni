//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-24 23:20:26
//

#include <rhi/render_context.hpp>
#include <renderer/scene.hpp>

#include "renderer/hot_reloadable_pipeline.hpp"

class ChromaticAberration
{
public:
    ChromaticAberration(RenderContext::Ptr context, Texture::Ptr input);
    ~ChromaticAberration() = default;

    void Render(Scene& scene, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR);
    void OnUI();
    void Reconstruct();

private:
    RenderContext::Ptr _renderContext;
    
    HotReloadablePipeline _computePipeline;
    bool _enable = true;

    Texture::Ptr _inputHDR;
    glm::ivec3 _offsetInPixels = glm::ivec3(0, 0, 0);
};
