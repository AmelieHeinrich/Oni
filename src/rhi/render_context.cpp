/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:51:35
 */

#include "render_context.hpp"

#include <core/log.hpp>

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx12.h>

#include <shader/bytecode.hpp>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

RenderContext::RenderContext(std::shared_ptr<Window> hwnd)
    : _window(hwnd)
{
    _device = std::make_shared<Device>();

    _graphicsQueue = std::make_shared<CommandQueue>(_device, CommandQueueType::Graphics);
    _computeQueue = std::make_shared<CommandQueue>(_device, CommandQueueType::Compute);
    _copyQueue = std::make_shared<CommandQueue>(_device, CommandQueueType::Copy);

    _heaps.RTVHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::RenderTarget, 1024);
    _heaps.DSVHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::DepthTarget, 1024);
    _heaps.ShaderHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::ShaderResource, 1'000'000);
    _heaps.SamplerHeap = std::make_shared<DescriptorHeap>(_device, DescriptorHeapType::Sampler, 512);

    _allocator = std::make_shared<Allocator>(_device);

    _graphicsFence.Fence = std::make_shared<Fence>(_device);
    _computeFence.Fence = std::make_shared<Fence>(_device);
    _copyFence.Fence = std::make_shared<Fence>(_device);

    _graphicsFence.Value = 0;
    _computeFence.Value = 0;
    _copyFence.Value = 0;

    _swapChain = std::make_shared<SwapChain>(_device, _graphicsQueue, _heaps.RTVHeap, hwnd->GetHandle());

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        _commandBuffers[i] = std::make_shared<CommandBuffer>(_device, _heaps, CommandQueueType::Graphics);
        _frameValues[i] = 0;
    }

    _fontDescriptor = _heaps.ShaderHeap->Allocate();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& IO = ImGui::GetIO();
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    //IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark();
    ImGuiStyle& Style = ImGui::GetStyle();

    if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        Style.WindowRounding = 0.0f;
        Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
    ImGui_ImplWin32_EnableDpiAwareness();
    ImGui_ImplDX12_Init(_device->GetDevice(), FRAMES_IN_FLIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, _heaps.ShaderHeap->GetHeap(), _fontDescriptor.CPU, _fontDescriptor.GPU);
    ImGui_ImplWin32_Init(hwnd->GetHandle());

    ShaderBytecode bytecode;
    ShaderCompiler::CompileShader("shaders/MipMaps/GenerateCompute.hlsl", "Main", ShaderType::Compute, bytecode);

    _mipmapPipeline = CreateComputePipeline(bytecode);
    _mipmapSampler = CreateSampler(SamplerAddress::Clamp, SamplerFilter::Linear, true, 0);
}

RenderContext::~RenderContext()
{
    WaitForPreviousFrame();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    _heaps.ShaderHeap->Free(_fontDescriptor);
}

void RenderContext::Resize(uint32_t width, uint32_t height)
{
    WaitForPreviousFrame();

    if (_swapChain) {
        _swapChain->Resize(width, height);
        Logger::Info("D3D12: Resized to (%u, %u)", width, height);
    }
}

void RenderContext::BeginFrame()
{
    _frameIndex = _swapChain->AcquireImage();
    _graphicsFence.Fence->Wait(_frameValues[_frameIndex], 10'000'000);

    _allocator->GetAllocator()->SetCurrentFrameIndex(_frameIndex);
}

void RenderContext::EndFrame()
{
    _frameValues[_frameIndex] = _graphicsFence.Fence->Value();
}

void RenderContext::Present(bool vsync)
{
    _swapChain->Present(vsync);
}

void RenderContext::FlushQueues()
{
    WaitForPreviousHostSubmit(CommandQueueType::Graphics);
    WaitForPreviousHostSubmit(CommandQueueType::Compute);
    WaitForPreviousHostSubmit(CommandQueueType::Copy);
}

void RenderContext::WaitForPreviousFrame()
{
    uint64_t wait = _graphicsFence.Fence->Signal(_graphicsQueue);
    _graphicsFence.Fence->Wait(wait, 10'000'000);
}

void RenderContext::WaitForPreviousHostSubmit(CommandQueueType type)
{
    switch (type) {
        case CommandQueueType::Graphics: {
            _graphicsFence.Fence->Wait(_graphicsFence.Value, 10'000'000);
            break;
        }
        case CommandQueueType::Compute:  {
            _computeFence.Fence->Wait(_computeFence.Value, 10'000'000);
            break;
        }
        case CommandQueueType::Copy: {
            _copyFence.Fence->Wait(_copyFence.Value, 10'000'000);
            break;
        }
    }
}

void RenderContext::WaitForPreviousDeviceSubmit(CommandQueueType type)
{
    switch (type) {
        case CommandQueueType::Graphics: {
            _graphicsQueue->Wait(_graphicsFence.Fence, _graphicsFence.Value);
            break;
        }
        case CommandQueueType::Compute:  {
            _computeQueue->Wait(_computeFence.Fence, _computeFence.Value);
            break;
        }
        case CommandQueueType::Copy: {
            _copyQueue->Wait(_copyFence.Fence, _copyFence.Value);
            break;
        }
    }
}

void RenderContext::ExecuteCommandBuffers(const std::vector<CommandBuffer::Ptr>& buffers, CommandQueueType type)
{
    switch (type) {
        case CommandQueueType::Graphics: {
            _graphicsQueue->Submit(buffers);
            _graphicsFence.Value = _graphicsFence.Fence->Signal(_graphicsQueue);
            WaitForPreviousHostSubmit(CommandQueueType::Graphics);
            break;
        }
        case CommandQueueType::Compute:  {
            _computeQueue->Submit(buffers);
            _computeFence.Value = _computeFence.Fence->Signal(_computeQueue);
            WaitForPreviousHostSubmit(CommandQueueType::Compute);
            break;
        }
        case CommandQueueType::Copy: {
            _copyQueue->Submit(buffers);
            _copyFence.Value = _copyFence.Fence->Signal(_copyQueue);
            WaitForPreviousHostSubmit(CommandQueueType::Copy);
            break;
        }
    }
}

CommandBuffer::Ptr RenderContext::GetCurrentCommandBuffer()
{
    return _commandBuffers[_frameIndex];
}

Texture::Ptr RenderContext::GetBackBuffer()
{
    return _swapChain->GetTexture(_frameIndex);
}

Buffer::Ptr RenderContext::CreateBuffer(uint64_t size, uint64_t stride, BufferType type, bool readback, const std::string& name)
{
    return std::make_shared<Buffer>(_device, _allocator, _heaps, size, stride, type, readback, name);
}

GraphicsPipeline::Ptr RenderContext::CreateGraphicsPipeline(GraphicsPipelineSpecs& specs)
{
    return std::make_shared<GraphicsPipeline>(_device, specs);
}

ComputePipeline::Ptr RenderContext::CreateComputePipeline(ShaderBytecode& shader)
{
    return std::make_shared<ComputePipeline>(_device, shader);
}

Texture::Ptr RenderContext::CreateTexture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, bool mips, const std::string& name)
{
    return std::make_shared<Texture>(_device, _allocator, _heaps, width, height, format, usage, mips, name);
}

Sampler::Ptr RenderContext::CreateSampler(SamplerAddress address, SamplerFilter filter, bool mips, int anisotropyLevel)
{
    return std::make_shared<Sampler>(_device, _heaps, address, filter, mips, anisotropyLevel);
}

CubeMap::Ptr RenderContext::CreateCubeMap(uint32_t width, uint32_t height, TextureFormat format, const std::string& name)
{
    return std::make_shared<CubeMap>(_device, _allocator, _heaps, width, height, format, name);
}

CommandBuffer::Ptr RenderContext::CreateCommandBuffer(CommandQueueType type)
{
    return std::make_shared<CommandBuffer>(_device, _heaps, type);
}

Uploader RenderContext::CreateUploader()
{
    return Uploader(_device, _allocator, _heaps);
}

void RenderContext::FlushUploader(Uploader& uploader)
{
    uploader._commandBuffer->Begin();

    for (auto& command : uploader._commands) {
        auto cmdBuf = uploader._commandBuffer;

        switch (command.type) {
            case Uploader::UploadCommandType::HostToDeviceShared: {
                void *pData;
                command.destBuffer->Map(0, 0, &pData);
                memcpy(pData, command.data, command.size);
                command.destBuffer->Unmap(0, 0);
                break;
            }
            case Uploader::UploadCommandType::BufferToBuffer:
            case Uploader::UploadCommandType::HostToDeviceLocal: {
                cmdBuf->CopyBufferToBuffer(command.destBuffer, command.sourceBuffer);
                break;
            }
            case Uploader::UploadCommandType::TextureToTexture: {
                cmdBuf->CopyTextureToTexture(command.destTexture, command.sourceTexture);
                break;
            }
            case Uploader::UploadCommandType::TextureToBuffer: {
                cmdBuf->CopyTextureToBuffer(command.destBuffer, command.sourceTexture);
                break;
            }
            case Uploader::UploadCommandType::BufferToTexture: {
                auto state = command.destTexture->GetState();
                cmdBuf->ImageBarrier(command.destTexture, TextureLayout::CopyDest);
                cmdBuf->CopyBufferToTexture(command.destTexture, command.sourceBuffer);
                cmdBuf->ImageBarrier(command.destTexture, TextureLayout(state));
                break;
            }
            default: {
                break;
            }
        } 
    }

    uploader._commandBuffer->End();
    ExecuteCommandBuffers({ uploader._commandBuffer }, CommandQueueType::Graphics);
    WaitForPreviousHostSubmit(CommandQueueType::Graphics);
    uploader._commands.clear();
}

void RenderContext::GenerateMips(Texture::Ptr texture)
{
    texture->BuildStorage();

    std::vector<Buffer::Ptr> buffers;
    buffers.resize(texture->GetMips() - 1);
    CommandBuffer::Ptr cmdBuf = CreateCommandBuffer(CommandQueueType::Graphics);

    for (int i = 0; i < texture->GetMips() - 1; i++) {
        Buffer::Ptr mipParameter = CreateBuffer(256, 0, BufferType::Constant, false, "Mipmap Buffer");
        mipParameter->BuildConstantBuffer();
        buffers[i] = mipParameter;
    }

    cmdBuf->Begin();
    cmdBuf->ImageBarrier(texture, TextureLayout::Storage);
    cmdBuf->BindComputePipeline(_mipmapPipeline);
    for (int i = 0; i < texture->GetMips() - 1; i++) {
        uint32_t mipWidth = (texture->GetWidth() * std::pow(0.5f, i + 1));
        uint32_t mipHeight = (texture->GetHeight() * std::pow(0.5f, i + 1));
        glm::vec4 data(mipWidth, mipHeight, 0, 0);

        void *pData;
        buffers[i]->Map(0, 0, &pData);
        memcpy(pData, glm::value_ptr(data), sizeof(data));
        buffers[i]->Unmap(0, 0);

        cmdBuf->BindComputeShaderResource(texture, 0, i);
        cmdBuf->BindComputeStorageTexture(texture, 1, i + 1);
        cmdBuf->BindComputeSampler(_mipmapSampler, 2);
        cmdBuf->BindComputeConstantBuffer(buffers[i], 3);
        cmdBuf->Dispatch(std::max(mipWidth / 8, 1u), std::max(mipHeight / 8, 1u), 1);
    }
    cmdBuf->ImageBarrier(texture, TextureLayout::ShaderResource);
    cmdBuf->End();
    ExecuteCommandBuffers({ cmdBuf }, CommandQueueType::Graphics);
}

void RenderContext::OnGUI()
{
    _allocator->OnGUI();
}

void RenderContext::OnOverlay()
{
    static bool p_open = true;

    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos, window_pos_pivot;
    window_pos.x = (work_pos.x + PAD);
    window_pos.y = (work_pos.y + PAD);
    window_pos_pivot.x = 0.0f;
    window_pos_pivot.y = 0.0f;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    window_flags |= ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin("Example: Simple overlay", &p_open, window_flags);
    ImGui::Text("Oni: An experimental renderer written by Amélie Heinrich");
    ImGui::Text("Version 0.0.1");
    ImGui::Text("Renderer: D3D12");
    ImGui::Text("%s", _device->GetName().c_str());
    ImGui::End();
}
