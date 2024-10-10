/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:34:12
 */

#include "renderer.hpp"
#include "core/log.hpp"
#include "core/texture_file.hpp"
#include "core/texture_compressor.hpp"

#include <sstream>
#include <algorithm>
#include <ctime>

#include <ImGui/imgui.h>
#include <stb/stb_image_write.h>
#include <optick.h>

Renderer::Renderer(RenderContext::Ptr context)
    : _renderContext(context)
{
    // Create
    if (context->GetDevice()->GetFeatures().Raytracing) {
        _rtShadows = std::make_shared<RTShadows>(context);
    }
    _shadows = std::make_shared<Shadows>(context, ShadowMapResolution::Ultra);
    _ssao = std::make_shared<SSAO>(context);
    _deferred = std::make_shared<Deferred>(context);
    _envMapForward = std::make_shared<EnvMapForward>(context, _deferred->GetOutput(), _deferred->GetDepthBuffer());

    _taa = std::make_shared<TemporalAntiAliasing>(context, _deferred->GetOutput());
    _motionBlur = std::make_shared<MotionBlur>(context, _deferred->GetOutput());
    _chromaticAberration = std::make_shared<ChromaticAberration>(context, _deferred->GetOutput());
    _bloom = std::make_shared<Bloom>(context, _deferred->GetOutput());
    _colorCorrection = std::make_shared<ColorCorrection>(context, _deferred->GetOutput());
    _filmGrain = std::make_shared<FilmGrain>(context, _deferred->GetOutput());
    _autoExposure = std::make_shared<AutoExposure>(context, _deferred->GetOutput());
    _tonemapping = std::make_shared<Tonemapping>(context, _deferred->GetOutput());

    _debugRenderer = std::make_shared<DebugRenderer>(context, _tonemapping->GetOutput());

    // Connect
    _ssao->SetDepthBuffer(_deferred->GetDepthBuffer());
    _ssao->SetNormalBuffer(_deferred->GetNormalBuffer());

    _deferred->ConnectEnvironmentMap(_envMapForward->GetEnvMap());
    _deferred->ConnectShadowMap(_shadows->GetOutput());
    _deferred->ConnectSSAO(_ssao->GetOutput());

    _taa->SetVelocityBuffer(_deferred->GetVelocityBuffer());
    _motionBlur->SetVelocityBuffer(_deferred->GetVelocityBuffer());
    _bloom->ConnectEmissiveBuffer(_deferred->GetEmissiveBuffer());

    _debugRenderer->SetVelocityBuffer(_deferred->GetVelocityBuffer());

    //
    DebugRenderer::SetDebugRenderer(_debugRenderer);
}

Renderer::~Renderer()
{
    
}
    
void Renderer::Render(Scene& scene, uint32_t width, uint32_t height, float dt)
{
    CommandBuffer::Ptr cmdBuf = _renderContext->GetCurrentCommandBuffer();

    scene.Update(_renderContext);

    _deferred->ShouldJitter(_taa->IsEnabled());

    {
        OPTICK_EVENT("Frame Render");

        _stats.PushFrameTime("Shadows", [this, &scene, width, height]() {
            _shadows->Render(scene, width, height);
        });
        _stats.PushFrameTime("GBuffer", [this, &scene, width, height]() {
            if (_deferred->UseMeshShaders()) {
                _deferred->GBufferPassMesh(scene, width, height);
            } else {
                _deferred->GBufferPassClassic(scene, width, height);
            }
        });
        _stats.PushFrameTime("SSAO", [this, &scene, width, height]() {
            _ssao->Render(scene, width, height);
        });
        _stats.PushFrameTime("Lighting", [this, &scene, width, height]() {
            _deferred->LightingPass(scene, width, height);
        });
        _stats.PushFrameTime("Environment Map", [this, &scene, width, height]() {
            _envMapForward->Render(scene, width, height);
        });

        _stats.PushFrameTime("Temporal Anti-Aliasing", [this, &scene, width, height]() {
            _taa->Render(scene, width, height);
        });
        _stats.PushFrameTime("Motion Blur", [this, &scene, width, height]() {
            _motionBlur->Render(scene, width, height);
        });
        _stats.PushFrameTime("Chromatic Aberration", [this, &scene, width, height]() {
            _chromaticAberration->Render(scene, width, height);
        });
        _stats.PushFrameTime("Bloom", [this, &scene, width, height]() {
            _bloom->Render(scene, width, height);
        });
        _stats.PushFrameTime("Color Correction", [this, &scene, width, height]() {
            _colorCorrection->Render(scene, width, height);
        });
        _stats.PushFrameTime("Film Grain", [this, &scene, width, height, dt]() {
            _filmGrain->Render(scene, width, height, dt);
        });
        _stats.PushFrameTime("Auto Exposure", [this, &scene, width, height, dt]() {
            _autoExposure->Render(scene, width, height, dt);
        });
        _stats.PushFrameTime("Tonemapping", [this, &scene, width, height]() {
            _tonemapping->Render(scene, width, height);
        });

        _stats.PushFrameTime("Debug Renderer", [this, &scene, width, height]() {
            _debugRenderer->Flush(scene, width, height);
        });
    }

    {
        CommandBuffer::Ptr cmdBuf = _renderContext->GetCurrentCommandBuffer();
        Texture::Ptr backbuffer = _renderContext->GetBackBuffer();

        OPTICK_GPU_CONTEXT(cmdBuf->GetCommandList());
        OPTICK_GPU_EVENT("Copy to Backbuffer");

        _stats.PushFrameTime("Copy to Backbuffer", [this, cmdBuf, backbuffer]() {
            cmdBuf->BeginEvent("Copy To Backbuffer");
            cmdBuf->ImageBarrier(backbuffer, TextureLayout::CopyDest);
            cmdBuf->ImageBarrier(_debugRenderer->GetOutput(), TextureLayout::CopySource);
            cmdBuf->CopyTextureToTexture(backbuffer, _debugRenderer->GetOutput());
            cmdBuf->ImageBarrier(backbuffer, TextureLayout::Present);
            cmdBuf->ImageBarrier(_debugRenderer->GetOutput(), TextureLayout::ShaderResource);
            cmdBuf->EndEvent();
        });
    }
}

// Not used
void Renderer::Resize(uint32_t width, uint32_t height)
{
    _shadows->Resize(width, height);
    _ssao->Resize(width, height);
    _deferred->Resize(width, height);
    _envMapForward->Resize(width, height, _deferred->GetOutput(), _deferred->GetDepthBuffer());

    _taa->Resize(width, height);
    _motionBlur->Resize(width, height);
    _chromaticAberration->Resize(width, height, _deferred->GetOutput());
    _bloom->Resize(width, height, _deferred->GetOutput());
    _colorCorrection->Resize(width, height, _deferred->GetOutput());
    _filmGrain->Resize(width, height, _deferred->GetOutput());
    _autoExposure->Resize(width, height, _deferred->GetOutput());
    _tonemapping->Resize(width, height, _deferred->GetOutput());

    _debugRenderer->Resize(width, height, _tonemapping->GetOutput());
}

void Renderer::OnUI()
{
    ImGui::Begin("Renderer Settings");

    _shadows->OnUI();
    _ssao->OnUI();
    _deferred->OnUI();
    _envMapForward->OnUI();

    _taa->OnUI();
    _motionBlur->OnUI();
    _chromaticAberration->OnUI();
    _bloom->OnUI();
    _colorCorrection->OnUI();
    _filmGrain->OnUI();
    _autoExposure->OnUI();
    _tonemapping->OnUI();
    
    _debugRenderer->OnUI();

    ImGui::End();
}

void Renderer::Screenshot(Texture::Ptr screenshotTexture, TextureLayout newLayout)
{
    OPTICK_EVENT("Screenshot");

    _renderContext->WaitForGPU();

    Texture::Ptr toScreenshot = screenshotTexture != nullptr ? screenshotTexture : _tonemapping->GetOutput();
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
    memset(Result, 0x00000000, toScreenshot->GetWidth() * toScreenshot->GetHeight() * 4);

    CommandBuffer::Ptr cmdBuffer = _renderContext->CreateCommandBuffer(CommandQueueType::Graphics, false);

    OPTICK_GPU_CONTEXT(cmdBuffer->GetCommandList());
    OPTICK_GPU_EVENT("Screenshot");

    cmdBuffer->Begin(false);
    cmdBuffer->ImageBarrier(toScreenshot, TextureLayout::CopySource);
    cmdBuffer->CopyTextureToBuffer(textureBuffer, toScreenshot);
    cmdBuffer->ImageBarrier(toScreenshot, newLayout);
    cmdBuffer->End();
    _renderContext->ExecuteCommandBuffers({ cmdBuffer }, CommandQueueType::Graphics);

    _renderContext->WaitForGPU();

    void *pData;
    textureBuffer->Map(0, 0, &pData);
    memcpy(Result, pData, toScreenshot->GetWidth() * toScreenshot->GetHeight() * 4);
    textureBuffer->Unmap(0, 0);

    stbi_write_png(String.c_str(), toScreenshot->GetWidth(), toScreenshot->GetHeight(), 4, Result, toScreenshot->GetWidth() * 4);
    Logger::Info("Saved screenshot at %s", String.c_str());
    delete Result;
}

void Renderer::Reconstruct()
{
    _shadows->Reconstruct();
    _ssao->Reconstruct();
    _deferred->Reconstruct();
    _envMapForward->Reconstruct();

    _taa->Reconstruct();
    _motionBlur->Reconstruct();
    _chromaticAberration->Reconstruct();
    _bloom->Reconstruct();
    _colorCorrection->Reconstruct();
    _filmGrain->Reconstruct();
    _autoExposure->Reconstruct();
    _tonemapping->Reconstruct();

    _debugRenderer->Reconstruct();
}
