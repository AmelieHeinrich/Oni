/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 17:24:42
 */

#include "tonemapping.hpp"

Tonemapping::Tonemapping(RenderContext::Ptr context, Texture::Ptr inputHDR)
    : _renderContext(context), _inputHDR(inputHDR)
{
    uint32_t width, height;
    context->GetWindow()->GetSize(width, height);

    _outputLDR = _renderContext->CreateTexture(width, height, TextureFormat::RGBA8, TextureUsage::Storage);
    _outputLDR->BuildStorage();

    ShaderBytecode bytecode;
    ShaderCompiler::CompileShader("shaders/Tonemapping/TonemappingCompute.hlsl", "Main", ShaderType::Compute, bytecode);
    _computePipeline = _renderContext->CreateComputePipeline(bytecode);
}

Tonemapping::~Tonemapping()
{
}

void Tonemapping::Render(Scene& scene, uint32_t width, uint32_t height)
{
    CommandBuffer::Ptr cmdBuf = _renderContext->GetCurrentCommandBuffer();

    cmdBuf->Begin();

    cmdBuf->ImageBarrier(_inputHDR, TextureLayout::ShaderResource);
    cmdBuf->ImageBarrier(_outputLDR, TextureLayout::Storage);
    
    cmdBuf->BindComputePipeline(_computePipeline);
    cmdBuf->BindComputeShaderResource(_inputHDR, 0);
    cmdBuf->BindComputeStorageTexture(_outputLDR, 1);
    cmdBuf->Dispatch(width / 31, height / 31, 1);
    
    cmdBuf->End();
    _renderContext->ExecuteCommandBuffers({ cmdBuf }, CommandQueueType::Graphics);   
}

void Tonemapping::Resize(uint32_t width, uint32_t height, Texture::Ptr inputHDR)
{
    _inputHDR = inputHDR;

    _outputLDR.reset();
    _outputLDR = _renderContext->CreateTexture(width, height, TextureFormat::RGBA8, TextureUsage::Storage);
    _outputLDR->BuildStorage();
}

void Tonemapping::OnUI()
{

}
