/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:34:12
 */

#include "renderer.hpp"

Renderer::Renderer(RenderContext::Ptr context)
    : _renderContext(context)
{
    _forward = std::make_shared<Forward>(context);
    _envMapForward = std::make_shared<EnvMapForward>(context, _forward->GetOutput(), _forward->GetDepthBuffer());
    _tonemapping = std::make_shared<Tonemapping>(context, _forward->GetOutput());

    _forward->ConnectEnvironmentMap(_envMapForward->GetEnvMap());
}

Renderer::~Renderer()
{
    
}
    
void Renderer::Render(Scene& scene, uint32_t width, uint32_t height)
{
    _forward->Render(scene, width, height);
    _envMapForward->Render(scene, width, height);
    _tonemapping->Render(scene, width, height);

    CommandBuffer::Ptr cmdBuf = _renderContext->GetCurrentCommandBuffer();
    Texture::Ptr backbuffer = _renderContext->GetBackBuffer();

    cmdBuf->Begin();
    cmdBuf->ImageBarrier(backbuffer, TextureLayout::CopyDest);
    cmdBuf->ImageBarrier(_tonemapping->GetOutput(), TextureLayout::CopySource);
    cmdBuf->CopyTextureToTexture(backbuffer, _tonemapping->GetOutput());
    cmdBuf->ImageBarrier(backbuffer, TextureLayout::Present);
    cmdBuf->End();
    _renderContext->ExecuteCommandBuffers({ cmdBuf }, CommandQueueType::Graphics);
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    _forward->Resize(width, height);
    _envMapForward->Resize(width, height, _forward->GetOutput(), _forward->GetDepthBuffer());
    _tonemapping->Resize(width, height, _forward->GetOutput());
}

void Renderer::OnUI()
{

}
