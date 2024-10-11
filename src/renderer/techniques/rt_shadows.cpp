//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 21:36:11
//

#include "rt_shadows.hpp"

RTShadows::RTShadows(RenderContext::Ptr context)
    : _context(context), _rtPipeline(PipelineType::Raytracing)
{
    uint32_t width, height;
    context->GetWindow()->GetSize(width, height);

    _rtPipeline.RTSpecs.PayloadSize = 2 * sizeof(uint32_t);
    _rtPipeline.RTSpecs.MaxTraceRecursionDepth = 3;

    _rtPipeline.SignatureInfo = {
        { RootSignatureEntry::PushConstants, RootSignatureEntry::SRV },
        4 * sizeof(uint32_t)
    };
    _rtPipeline.ReflectRootSignature(false);
    _rtPipeline.AddShaderWatch("shaders/Raytracing/Shadows/RTShadowsLib.hlsl", "", ShaderType::Raytracing);
    _rtPipeline.Build(context);

    _output = context->CreateTexture(width, height, TextureFormat::RGBA16Float, TextureUsage::RenderTarget, false, "[RT SHADOWS] Shadow map");
    _output->BuildShaderResource();
    _output->BuildStorage();
    _output->BuildRenderTarget();

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        _cameraBuffers[i] = context->CreateBuffer(256, 0, BufferType::Constant, false, "[RT SHADOWS] Camera Buffer");
        _cameraBuffers[i]->BuildConstantBuffer();

        _lightBuffers[i] = context->CreateBuffer(24832, 0, BufferType::Constant, false, "[RT SHADOWS] Light Buffer");
        _lightBuffers[i]->BuildConstantBuffer();
    }
}

void RTShadows::Render(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();
    uint32_t frameIndex = _context->GetBackBufferIndex();

    // Update data
    struct CameraData {
        glm::mat4 InvView;
        glm::mat4 InvProj;

        glm::vec3 CameraPosition;
        float Pad;
    };
    CameraData cam;
    cam.InvView = glm::inverse(scene.Camera.View());
    cam.InvProj = glm::inverse(scene.Camera.Projection());
    cam.CameraPosition = scene.Camera.GetPosition();
    cam.Pad = 0.0f;

    void *pData;
    _lightBuffers[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &scene.Lights.GetGPUData(), sizeof(LightSettings::GPUData));
    _lightBuffers[frameIndex]->Unmap(0, 0);

    _cameraBuffers[frameIndex]->Map(0, 0, &pData);
    memcpy(pData, &cam, sizeof(cam));
    _cameraBuffers[frameIndex]->Unmap(0, 0);

    commandBuffer->BeginEvent("RT Shadows");

    // Clear Output
    commandBuffer->BeginEvent("RT Shadows Clear");
    commandBuffer->ImageBarrierBatch({
        { _output, TextureLayout::RenderTarget }
    });
    commandBuffer->ClearRenderTarget(_output, 1.0f, 1.0f, 1.0f, 1.0f);
    commandBuffer->ImageBarrierBatch({
        { _output, TextureLayout::Storage }
    });
    commandBuffer->EndEvent();

    if (_enable) {
        struct Data {
            uint32_t Scene;
            uint32_t Camera;
            uint32_t Light;
            uint32_t Output;
        };
        Data data = {
            scene.TLAS->SRV(),
            _cameraBuffers[frameIndex]->CBV(),
            _lightBuffers[frameIndex]->CBV(),
            _output->UAV()
        };

        commandBuffer->BeginEvent("RT Shadows");
        commandBuffer->ImageBarrierBatch({
            { _output, TextureLayout::Storage }
        });
        commandBuffer->BindRaytracingPipeline(_rtPipeline.RTPipeline);
        commandBuffer->PushConstantsCompute(&data, sizeof(data), 0);
        commandBuffer->BindComputeAccelerationStructure(scene.TLAS, 1);
        commandBuffer->TraceRays(width, height);
        commandBuffer->ImageBarrierBatch({
            { _output, TextureLayout::ShaderResource }
        });
        commandBuffer->EndEvent();
    }

    commandBuffer->EndEvent();
}

void RTShadows::OnUI()
{
    if (ImGui::TreeNodeEx("RT Shadows", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Enable", &_enable);
        ImGui::TreePop();
    }
}

void RTShadows::Reconstruct()
{
    _rtPipeline.CheckForRebuild(_context, "RT Shadows");
}
