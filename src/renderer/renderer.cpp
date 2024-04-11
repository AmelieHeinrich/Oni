/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:34:12
 */

#include "renderer.hpp"

#include <sstream>
#include <algorithm>
#include <ctime>

#include <ImGui/imgui.h>
#include <stb/stb_image_write.h>

Renderer::Renderer(RenderContext::Ptr context)
    : _renderContext(context)
{
    _forward = std::make_shared<Forward>(context);
    _envMapForward = std::make_shared<EnvMapForward>(context, _forward->GetOutput(), _forward->GetDepthBuffer());
    _colorCorrection = std::make_shared<ColorCorrection>(context, _forward->GetOutput());
    _autoExposure = std::make_shared<AutoExposure>(context, _forward->GetOutput());
    _tonemapping = std::make_shared<Tonemapping>(context, _forward->GetOutput());

    _forward->ConnectEnvironmentMap(_envMapForward->GetEnvMap());
}

Renderer::~Renderer()
{
    
}
    
void Renderer::Render(Scene& scene, uint32_t width, uint32_t height, float dt)
{
    _forward->Render(scene, width, height);
    _envMapForward->Render(scene, width, height);
    _colorCorrection->Render(scene, width, height);
    _autoExposure->Render(scene, width, height, dt);
    _tonemapping->Render(scene, width, height);

    CommandBuffer::Ptr cmdBuf = _renderContext->GetCurrentCommandBuffer();
    Texture::Ptr backbuffer = _renderContext->GetBackBuffer();

    cmdBuf->Begin();
    cmdBuf->ImageBarrier(backbuffer, TextureLayout::CopyDest);
    cmdBuf->ImageBarrier(_tonemapping->GetOutput(), TextureLayout::CopySource);
    cmdBuf->CopyTextureToTexture(backbuffer, _tonemapping->GetOutput());
    cmdBuf->ImageBarrier(backbuffer, TextureLayout::Present);
    cmdBuf->ImageBarrier(_tonemapping->GetOutput(), TextureLayout::ShaderResource);
    cmdBuf->End();
    _renderContext->ExecuteCommandBuffers({ cmdBuf }, CommandQueueType::Graphics);

    if (ImGui::IsKeyPressed(ImGuiKey_F12, false)) {
        Screenshot();
    }
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    _forward->Resize(width, height);
    _envMapForward->Resize(width, height, _forward->GetOutput(), _forward->GetDepthBuffer());
    _colorCorrection->Resize(width, height, _forward->GetOutput());
    _autoExposure->Resize(width, height, _forward->GetOutput());
    _tonemapping->Resize(width, height, _forward->GetOutput());
}

void Renderer::OnUI()
{
    ImGui::Begin("Renderer Settings");

    _forward->OnUI();
    _envMapForward->OnUI();
    _colorCorrection->OnUI();
    _autoExposure->OnUI();
    _tonemapping->OnUI();

    ImGui::End();
}

void Renderer::Screenshot()
{
    _renderContext->WaitForPreviousHostSubmit(CommandQueueType::Graphics);

    Texture::Ptr toScreenshot = _tonemapping->GetOutput();
    Buffer::Ptr textureBuffer = _renderContext->CreateBuffer(toScreenshot->GetWidth() * toScreenshot->GetHeight() * 4, 0, BufferType::Copy, true, "Screenshot Buffer");

    std::stringstream Stream;
    time_t RawTime;
    time(&RawTime);
    tm *TimeInfo = localtime(&RawTime);
    Stream << "screenshots/engine/" << "Screenshot " << asctime(TimeInfo);
    std::string String = Stream.str();
    std::replace(String.begin(), String.end(), ':', '_');
    String.replace(String.size() - 1, 4, ".png");

    uint8_t *Result = new uint8_t[toScreenshot->GetWidth() * toScreenshot->GetHeight() * 4];
    memset(Result, 0xffffffff, toScreenshot->GetWidth() * toScreenshot->GetHeight() * 4);

    CommandBuffer::Ptr cmdBuffer = _renderContext->CreateCommandBuffer(CommandQueueType::Graphics);
    cmdBuffer->Begin();
    cmdBuffer->ImageBarrier(toScreenshot, TextureLayout::CopySource);
    cmdBuffer->CopyTextureToBuffer(textureBuffer, toScreenshot);
    cmdBuffer->ImageBarrier(toScreenshot, TextureLayout::ShaderResource);
    cmdBuffer->End();
    _renderContext->ExecuteCommandBuffers({ cmdBuffer }, CommandQueueType::Graphics);

    void *pData;
    textureBuffer->Map(0, 0, &pData);
    memcpy(Result, pData, toScreenshot->GetWidth() * toScreenshot->GetHeight() * 4);
    textureBuffer->Unmap(0, 0);

    stbi_write_png(String.c_str(), toScreenshot->GetWidth(), toScreenshot->GetHeight(), 4, Result, toScreenshot->GetWidth() * 4);
    delete Result;
}
