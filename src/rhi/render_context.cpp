/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:51:35
 */

#include "render_context.hpp"

#include <psapi.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx12.h>
#include <cmath>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <core/shader_bytecode.hpp>
#include <core/log.hpp>
#include <core/shader_loader.hpp>

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
        _commandBuffers[i] = std::make_shared<CommandBuffer>(_device, _allocator, _heaps, CommandQueueType::Graphics);
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
    IO.FontDefault = IO.Fonts->AddFontFromFileTTF("assets/fonts/GohuFont14NerdFontMono-Regular.ttf", 14); 

    ImGui::StyleColorsDark();
    ImGuiStyle& Style = ImGui::GetStyle();

    if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        Style.WindowRounding = 0.0f;
        Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    SetStyle();

    ImGui_ImplWin32_EnableDpiAwareness();
    ImGui_ImplDX12_Init(_device->GetDevice(), FRAMES_IN_FLIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, _heaps.ShaderHeap->GetHeap(), _fontDescriptor.CPU, _fontDescriptor.GPU);
    ImGui_ImplWin32_Init(hwnd->GetHandle());

    ShaderBytecode bytecode = ShaderLoader::GetFromCache("shaders/MipMaps/GenerateCompute.hlsl");

    _mipmapPipeline = CreateComputePipeline(bytecode, CreateDefaultRootSignature(sizeof(glm::vec4) * 2));
    _mipmapSampler = CreateSampler(SamplerAddress::Clamp, SamplerFilter::Linear, true, 0);

    ASBuilder::Init(_allocator, _device);

    WaitForGPU();
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
        Logger::Info("[D3D12] Resized to (%u, %u)", width, height);
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

ComputePipeline::Ptr RenderContext::CreateComputePipeline(ShaderBytecode& shader, RootSignature::Ptr rootSignature)
{
    return std::make_shared<ComputePipeline>(_device, shader, rootSignature);
}

MeshPipeline::Ptr RenderContext::CreateMeshPipeline(GraphicsPipelineSpecs& specs)
{
    return std::make_shared<MeshPipeline>(_device, specs);
}

RaytracingPipeline::Ptr RenderContext::CreateRaytracingPipeline(RaytracingPipelineSpecs& specs)
{
    return std::make_shared<RaytracingPipeline>(_device, _allocator, _heaps, specs);
}

Texture::Ptr RenderContext::CreateTexture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, bool mips, const std::string& name)
{
    return std::make_shared<Texture>(_device, _allocator, _heaps, width, height, format, usage, mips, name);
}

Sampler::Ptr RenderContext::CreateSampler(SamplerAddress address, SamplerFilter filter, bool mips, int anisotropyLevel)
{
    for (auto& sampler : _samplerCache) {
        if (sampler->Address() == address
        &&  sampler->AnisotropyLevel() == anisotropyLevel
        &&  sampler->Filter() == filter
        &&  sampler->HasMips() == mips) {
            return sampler;
        }
    }

    Sampler::Ptr result = std::make_shared<Sampler>(_device, _heaps, address, filter, mips, anisotropyLevel);
    _samplerCache.push_back(result);
    return result;
}

CubeMap::Ptr RenderContext::CreateCubeMap(uint32_t width, uint32_t height, TextureFormat format, int mips, const std::string& name)
{
    return std::make_shared<CubeMap>(_device, _allocator, _heaps, width, height, format, mips, name);
}

CommandBuffer::Ptr RenderContext::CreateCommandBuffer(CommandQueueType type, bool close)
{
    return std::make_shared<CommandBuffer>(_device, _allocator, _heaps, type, close);
}

BLAS::Ptr RenderContext::CreateBLAS(Buffer::Ptr vertexBuffer, Buffer::Ptr indexBuffer, int vertexCount, int indexCount, const std::string& name)
{
    return std::make_shared<BLAS>(vertexBuffer, indexBuffer, vertexCount, indexCount, name);
}

TLAS::Ptr RenderContext::CreateTLAS(Buffer::Ptr instanceBuffer, uint32_t numInstances, const std::string& name)
{
    return std::make_shared<TLAS>(_device, _allocator, _heaps, instanceBuffer, numInstances, name);
}

RootSignature::Ptr RenderContext::CreateRootSignature()
{
    return std::make_shared<RootSignature>(_device);
}

RootSignature::Ptr RenderContext::CreateRootSignature(RootSignatureBuildInfo& info)
{
    return std::make_shared<RootSignature>(_device, info);
}

Uploader RenderContext::CreateUploader()
{
    return Uploader(_device, _allocator, _heaps);
}

void RenderContext::FlushUploader(Uploader& uploader, CommandBuffer::Ptr cmdBuf)
{
    for (auto& command : uploader._commands) {
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
                cmdBuf->CopyBufferToTextureLOD(command.destTexture, command.sourceBuffer, 0);
                cmdBuf->ImageBarrier(command.destTexture, TextureLayout(state));
                break;
            }
            case Uploader::UploadCommandType::HostToDeviceCompressedTexture: {
                cmdBuf->ImageBarrier(command.destTexture, TextureLayout::CopyDest);
                cmdBuf->CopyTextureFileToTexture(command.destTexture, command.sourceBuffer, command.textureFile);
                cmdBuf->ImageBarrier(command.destTexture, TextureLayout::ShaderResource);
                break;
            }
            case Uploader::UploadCommandType::BuildBLAS: {
                cmdBuf->BuildAccelerationStructure(command.blas->_accelerationStructure, command.blas->_inputs);
                break;
            }
            case Uploader::UploadCommandType::BuildTLAS: {
                cmdBuf->BuildAccelerationStructure(command.tlas->_accelerationStructure, command.tlas->_inputs);
                break;
            }
            default: {
                break;
            }
        } 
    }
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
            case Uploader::UploadCommandType::HostToDeviceCompressedTexture: {
                cmdBuf->ImageBarrier(command.destTexture, TextureLayout::CopyDest);
                cmdBuf->CopyTextureFileToTexture(command.destTexture, command.sourceBuffer, command.textureFile);
                cmdBuf->ImageBarrier(command.destTexture, TextureLayout::ShaderResource);
                break;
            }
            case Uploader::UploadCommandType::BuildBLAS: {
                cmdBuf->BuildAccelerationStructure(command.blas->_accelerationStructure, command.blas->_inputs);
                break;
            }
            case Uploader::UploadCommandType::BuildTLAS: {
                cmdBuf->BuildAccelerationStructure(command.tlas->_accelerationStructure, command.tlas->_inputs);
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

    CommandBuffer::Ptr cmdBuf = CreateCommandBuffer(CommandQueueType::Graphics, false);

    cmdBuf->Begin(false);
    cmdBuf->BindComputePipeline(_mipmapPipeline);
    for (int i = 0; i < texture->GetMips() - 1; i++) {
        uint32_t mipWidth = (texture->GetWidth() * std::pow(0.5f, i + 1));
        uint32_t mipHeight = (texture->GetHeight() * std::pow(0.5f, i + 1));
        
        struct PushConstants {
            uint32_t SrcTexture;
            uint32_t DstTexture;
            uint32_t BilinearClamp;
            uint32_t Pad0;
            glm::vec4 MipSize;
        };
        PushConstants constants = {
            texture->SRV(i),
            texture->UAV(i + 1),
            _mipmapSampler->BindlesssSampler(),
            0,
            glm::vec4(mipWidth, mipHeight, 0, 0)
        };

        cmdBuf->ImageBarrierBatch({
            { texture, TextureLayout::ShaderResource, i },
            { texture, TextureLayout::Storage, i + 1 }
        });
        cmdBuf->PushConstantsCompute(&constants, sizeof(constants), 0);
        cmdBuf->Dispatch(std::max(mipWidth / 8, 1u), std::max(mipHeight / 8, 1u), 1);
    }
    cmdBuf->ImageBarrier(texture, TextureLayout::ShaderResource);
    cmdBuf->End();
    ExecuteCommandBuffers({ cmdBuf }, CommandQueueType::Graphics);
    WaitForGPU();
}

void RenderContext::GenerateMips(Texture::Ptr texture, CommandBuffer::Ptr cmdBuf)
{
    texture->BuildStorage();

    std::vector<Buffer::Ptr> buffers;
    buffers.resize(texture->GetMips());

    for (int i = 0; i < texture->GetMips(); i++) {
        buffers[i] = CreateBuffer(256, 0, BufferType::Constant, false, "Mipmap Buffer " + std::to_string(i));
        buffers[i]->BuildConstantBuffer();
    }

    cmdBuf->BindComputePipeline(_mipmapPipeline);
    for (int i = 0; i < texture->GetMips() - 1; i++) {
        uint32_t mipWidth = (texture->GetWidth() * std::pow(0.5f, i + 1));
        uint32_t mipHeight = (texture->GetHeight() * std::pow(0.5f, i + 1));
        glm::vec4 data(mipWidth, mipHeight, 0, 0);

        void *pData;
        buffers[i]->Map(0, 0, &pData);
        memcpy(pData, glm::value_ptr(data), sizeof(data));
        buffers[i]->Unmap(0, 0);

        cmdBuf->ImageBarrierBatch({
            { texture, TextureLayout::ShaderResource, i },
            { texture, TextureLayout::Storage, i + 1 }
        });
        cmdBuf->BindComputeShaderResource(texture, 0, i);
        cmdBuf->BindComputeStorageTexture(texture, 1, i + 1);
        cmdBuf->BindComputeSampler(_mipmapSampler, 2);
        cmdBuf->BindComputeConstantBuffer(buffers[i], 3);
        cmdBuf->Dispatch(std::max(mipWidth / 8, 1u), std::max(mipHeight / 8, 1u), 1);
        cmdBuf->ImageBarrier(texture, TextureLayout::ShaderResource, i + 1);
    }
}

RootSignature::Ptr RenderContext::CreateDefaultRootSignature(uint32_t pushConstantSize)
{
    return CreateRootSignature(RootSignatureBuildInfo {
        { RootSignatureEntry::PushConstants },
        pushConstantSize
    });
}

void RenderContext::OnGUI()
{
    _allocator->OnGUI();
}

void RenderContext::OnOverlay()
{
    static bool p_open = true;

    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking;
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

    ImGui::SetNextWindowBgAlpha(0.70f);
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

        float stupidVRAMPercetange = percentage / 100.0f;

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

        float stupidRAMPercetange = percentage / 100.0f;

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

        std::stringstream ss;
        ss << "Battery (" << percentage << "%%)";

        std::stringstream percentss;
        percentss << percentage << "%";
        
        ImGui::Text(ss.str().c_str());
        ImGui::ProgressBar(percentage / 100.0f, ImVec2(0, 0), percentss.str().c_str());
    }

    ImGui::End();
}

void RenderContext::SetStyle()
{
    ImGuiStyle & style = ImGui::GetStyle();
	ImVec4 * colors = style.Colors;
	
	/// 0 = FLAT APPEARENCE
	/// 1 = MORE "3D" LOOK
	int is3D = 0;
		
	colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_Border]                 = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_CheckMark]              = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
	colors[ImGuiCol_Button]                 = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_Header]                 = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
	colors[ImGuiCol_Separator]              = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

	style.PopupRounding = 3;

	style.WindowPadding = ImVec2(4, 4);
	style.FramePadding  = ImVec2(6, 4);
	style.ItemSpacing   = ImVec2(6, 2);

	style.ScrollbarSize = 18;

	style.WindowBorderSize = 1;
	style.ChildBorderSize  = 1;
	style.PopupBorderSize  = 1;
	style.FrameBorderSize  = is3D; 

	style.WindowRounding    = 3;
	style.ChildRounding     = 3;
	style.FrameRounding     = 3;
	style.ScrollbarRounding = 2;
	style.GrabRounding      = 3;

	#ifdef IMGUI_HAS_DOCK 
		style.TabBorderSize = is3D; 
		style.TabRounding   = 3;

		colors[ImGuiCol_DockingEmptyBg]     = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
		colors[ImGuiCol_Tab]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_TabHovered]         = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_TabActive]          = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
		colors[ImGuiCol_TabUnfocused]       = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
		colors[ImGuiCol_DockingPreview]     = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
	#endif
}
