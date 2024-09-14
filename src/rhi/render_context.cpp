/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:51:35
 */

#include "render_context.hpp"

#include <core/log.hpp>
#include <psapi.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx12.h>

#include <shader/bytecode.hpp>
#include <cmath>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

static float normalize(float value, float old_min, float old_max, float new_min, float new_max)
{
    return (new_max - new_min) * (value - old_min) / (old_max - old_min) + new_min;
}

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

    // Add fonts
    IO.FontDefault = IO.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", 14); 

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
    WaitForGPU();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    _heaps.ShaderHeap->Free(_fontDescriptor);
}

void RenderContext::Resize(uint32_t width, uint32_t height)
{
    WaitForGPU();

    if (_swapChain) {
        _swapChain->Resize(width, height);
        Logger::Info("D3D12: Resized to (%u, %u)", width, height);
    }
}

void RenderContext::Finish()
{
    const UINT64 currentFenceValue = _frameValues[_frameIndex];
    _graphicsQueue->Signal(_graphicsFence.Fence, currentFenceValue);

    _frameIndex = _swapChain->AcquireImage();

    if (_graphicsFence.Fence->CompletedValue() < _frameValues[_frameIndex]) {
        _graphicsFence.Fence->Wait(_frameValues[_frameIndex], INFINITE);
    }

    _frameValues[_frameIndex] = currentFenceValue + 1;
}

void RenderContext::Present(bool vsync)
{
    _swapChain->Present(vsync);
}

void RenderContext::WaitForGPU()
{
    _graphicsQueue->Signal(_graphicsFence.Fence, _frameValues[_frameIndex]);
    _graphicsFence.Fence->Wait(_frameValues[_frameIndex], 10'000'000);
    _frameValues[_frameIndex]++;
}

void RenderContext::ExecuteCommandBuffers(const std::vector<CommandBuffer::Ptr>& buffers, CommandQueueType type)
{
    switch (type) {
        case CommandQueueType::Graphics: {
            _graphicsQueue->Submit(buffers);
            break;
        }
        case CommandQueueType::Compute:  {
            _computeQueue->Submit(buffers);
            break;
        }
        case CommandQueueType::Copy: {
            _copyQueue->Submit(buffers);
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

CommandBuffer::Ptr RenderContext::CreateCommandBuffer(CommandQueueType type, bool close)
{
    return std::make_shared<CommandBuffer>(_device, _heaps, type, close);
}

Uploader RenderContext::CreateUploader()
{
    return Uploader(_device, _allocator, _heaps);
}

void RenderContext::FlushUploader(Uploader& uploader)
{
    uploader._commandBuffer->Begin(false);

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
                auto state = command.destTexture->GetState(0);
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
    WaitForGPU();
    uploader._commands.clear();
}

void RenderContext::GenerateMips(Texture::Ptr texture)
{
    texture->BuildStorage();

    std::vector<Buffer::Ptr> buffers;
    buffers.resize(texture->GetMips());
    CommandBuffer::Ptr cmdBuf = CreateCommandBuffer(CommandQueueType::Graphics, false);

    for (int i = 0; i < texture->GetMips(); i++) {
        buffers[i] = CreateBuffer(256, 0, BufferType::Constant, false, "Mipmap Buffer " + std::to_string(i));
        buffers[i]->BuildConstantBuffer();
    }

    cmdBuf->Begin(false);
    cmdBuf->BindComputePipeline(_mipmapPipeline);
    for (int i = 0; i < texture->GetMips() - 1; i++) {
        uint32_t mipWidth = (texture->GetWidth() * std::pow(0.5f, i + 1));
        uint32_t mipHeight = (texture->GetHeight() * std::pow(0.5f, i + 1));
        glm::vec4 data(mipWidth, mipHeight, 0, 0);

        void *pData;
        buffers[i]->Map(0, 0, &pData);
        memcpy(pData, glm::value_ptr(data), sizeof(data));
        buffers[i]->Unmap(0, 0);

        cmdBuf->ImageBarrier(texture, TextureLayout::ShaderResource, i);
        cmdBuf->ImageBarrier(texture, TextureLayout::Storage, i + 1);
        cmdBuf->BindComputeShaderResource(texture, 0, i);
        cmdBuf->BindComputeStorageTexture(texture, 1, i + 1);
        cmdBuf->BindComputeSampler(_mipmapSampler, 2);
        cmdBuf->BindComputeConstantBuffer(buffers[i], 3);
        cmdBuf->Dispatch(std::max(mipWidth / 8, 1u), std::max(mipHeight / 8, 1u), 1);
    }
    cmdBuf->ImageBarrier(texture, TextureLayout::ShaderResource);
    cmdBuf->End();
    ExecuteCommandBuffers({ cmdBuf }, CommandQueueType::Graphics);
    WaitForGPU();
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

    ImGui::Separator();

    // VRAM
    {
        Allocator::Stats stats = _allocator->GetStats();

        uint64_t total = stats.Total;
        uint64_t used = stats.Used;
        uint64_t percentage = (used * 100) / total;

        float stupidVRAMPercetange = normalize(stats.Total / stats.Used, 0.0f, 100.0f, 0.0f, 1.0f);

        std::stringstream ss;
        ss << "VRAM Usage (" << percentage << "%%): " << (((used / 1024.0F) / 1024.0f) / 1024.0f) << "gb/" << (((total / 1024.0F) / 1024.0f) / 1024.0f) << "gb";

        std::stringstream percentss;
        percentss << percentage << "%";

        ImGui::Text(ss.str().c_str());
        ImGui::ProgressBar(stupidVRAMPercetange, ImVec2(0, 0), percentss.str().c_str());
    }

    // RAM
    {
        HANDLE hProcess = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS pmc;
        GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));

        uint64_t totalRam = 0;
        GetPhysicallyInstalledSystemMemory(&totalRam);

        uint64_t total = totalRam * 1024;
        uint64_t used = pmc.WorkingSetSize;
        uint64_t percentage = (used * 100) / total;

        float stupidRAMPercetange = normalize(percentage, 0.0f, 100.0f, 0.0f, 1.0f);

        std::stringstream ss;
        ss << "RAM Usage (" << percentage << "%%): " << (((used / 1024.0F) / 1024.0f) / 1024.0f) << "gb/" << (((total / 1024.0F) / 1024.0f) / 1024.0f) << "gb";

        std::stringstream percentss;
        percentss << percentage << "%";
        
        ImGui::Text(ss.str().c_str());
        ImGui::ProgressBar(stupidRAMPercetange, ImVec2(0, 0), percentss.str().c_str());
    }

    // Battery
    {
        SYSTEM_POWER_STATUS status;
        GetSystemPowerStatus(&status);

        uint64_t percentage = status.BatteryLifePercent;

        float stupidBatteryPercetange = normalize(percentage, 0.0f, 100.0f, 0.0f, 1.0f);

        std::stringstream ss;
        ss << "Battery (" << percentage << "%%)";

        std::stringstream percentss;
        percentss << percentage << "%";
        
        ImGui::Text(ss.str().c_str());
        ImGui::ProgressBar(stupidBatteryPercetange, ImVec2(0, 0), percentss.str().c_str());
    }

    ImGui::End();
}
