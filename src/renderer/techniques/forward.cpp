/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 17:45:00
 */

#include "forward.hpp"

Forward::Forward(RenderContext::Ptr context)
    : _context(context)
{
    uint32_t width, height;
    context->GetWindow()->GetSize(width, height);

    _outputImage = context->CreateTexture(width, height, TextureFormat::RGBA32Float, TextureUsage::RenderTarget);
    _outputImage->BuildRenderTarget();
    _outputImage->BuildShaderResource();

    _depthBuffer = context->CreateTexture(width, height, TextureFormat::R32Depth, TextureUsage::DepthTarget);
    _depthBuffer->BuildDepthTarget();

    GraphicsPipelineSpecs specs;
    specs.FormatCount = 1;
    specs.Formats[0] = TextureFormat::RGBA32Float;
    specs.DepthFormat = TextureFormat::R32Depth;
    specs.Depth = DepthOperation::Less;
    specs.DepthEnabled = true;
    specs.Cull = CullMode::None;
    specs.Fill = FillMode::Solid;
    ShaderCompiler::CompileShader("shaders/Forward/ForwardVert.hlsl", "Main", ShaderType::Vertex, specs.Bytecodes[ShaderType::Vertex]);
    ShaderCompiler::CompileShader("shaders/Forward/ForwardFrag.hlsl", "Main", ShaderType::Fragment, specs.Bytecodes[ShaderType::Fragment]);

    _forwardPipeline = context->CreateGraphicsPipeline(specs);

    _sceneBuffer = context->CreateBuffer(256, 0, BufferType::Constant, false);
    _sceneBuffer->BuildConstantBuffer();
    
    _modelBuffer = context->CreateBuffer(256, 0, BufferType::Constant, false);
    _modelBuffer->BuildConstantBuffer();

    _sampler = context->CreateSampler(SamplerAddress::Wrap, SamplerFilter::Linear, 0);
}

Forward::~Forward()
{

}

void Forward::Render(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr commandBuffer = _context->GetCurrentCommandBuffer();

    glm::mat4 array[2] = {
        scene.View,
        scene.Projection
    };

    void *pData;
    _sceneBuffer->Map(0, 0, &pData);
    memcpy(pData, array, sizeof(glm::mat4) * 2);
    _sceneBuffer->Unmap(0, 0);

    commandBuffer->Begin();
    commandBuffer->ImageBarrier(_outputImage, TextureLayout::RenderTarget);
    commandBuffer->SetViewport(0, 0, width, height);
    commandBuffer->SetTopology(Topology::TriangleList);
    commandBuffer->BindRenderTargets({ _outputImage }, _depthBuffer);
    commandBuffer->ClearRenderTarget(_outputImage, 0.3f, 0.5f, 0.8f, 1.0f);
    commandBuffer->ClearDepthTarget(_depthBuffer);
    commandBuffer->BindGraphicsPipeline(_forwardPipeline);
    commandBuffer->BindGraphicsConstantBuffer(_sceneBuffer, 0);
    commandBuffer->BindGraphicsSampler(_sampler, 3);

    for (auto& model : scene.Models) {
        for (auto& primitive : model.Primitives) {
            auto& material = model.Materials[primitive.MaterialIndex];

            _modelBuffer->Map(0, 0, &pData);
            memcpy(pData, glm::value_ptr(primitive.Transform), sizeof(glm::mat4));
            _modelBuffer->Unmap(0, 0);

            commandBuffer->BindVertexBuffer(primitive.VertexBuffer);
            commandBuffer->BindIndexBuffer(primitive.IndexBuffer);
            commandBuffer->BindGraphicsConstantBuffer(_modelBuffer, 1);
            commandBuffer->BindGraphicsShaderResource(material.AlbedoTexture, 2);
            commandBuffer->DrawIndexed(primitive.IndexCount);
        }
    }

    commandBuffer->End();
    _context->ExecuteCommandBuffers({ commandBuffer }, CommandQueueType::Graphics);
}

void Forward::Resize(uint32_t width, uint32_t height)
{
    _outputImage.reset();
    _depthBuffer.reset();

    _outputImage = _context->CreateTexture(width, height, TextureFormat::RGBA32Float, TextureUsage::RenderTarget);
    _outputImage->BuildRenderTarget();
    _outputImage->BuildShaderResource();

    _depthBuffer = _context->CreateTexture(width, height, TextureFormat::R32Depth, TextureUsage::DepthTarget);
    _depthBuffer->BuildDepthTarget();
}

void Forward::OnUI()
{

}
