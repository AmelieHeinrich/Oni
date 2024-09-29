/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 18:17:33
 */

#include "device.hpp"

#include <core/log.hpp>

extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
extern "C" __declspec(dllexport) extern const uint32_t D3D12SDKVersion = 613;
extern "C" __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";

void GetHardwareAdapter(IDXGIFactory3 *Factory, IDXGIAdapter1 **RetAdapter, bool HighPerf)
{
    *RetAdapter = nullptr;
    IDXGIAdapter1* Adapter = 0;
    IDXGIFactory6* Factory6;
    
    if (SUCCEEDED(Factory->QueryInterface(IID_PPV_ARGS(&Factory6)))) 
    {
        for (uint32_t AdapterIndex = 0; SUCCEEDED(Factory6->EnumAdapterByGpuPreference(AdapterIndex, HighPerf == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&Adapter))); ++AdapterIndex) 
        {
            DXGI_ADAPTER_DESC1 Desc;
            Adapter->GetDesc1(&Desc);

            if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;

            if (SUCCEEDED(D3D12CreateDevice((IUnknown*)Adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device*), nullptr)))
                break;
        }
    }
    
    if (Adapter == nullptr) {
        for (uint32_t AdapterIndex = 0; SUCCEEDED(Factory->EnumAdapters1(AdapterIndex, &Adapter)); ++AdapterIndex) 
        {
            DXGI_ADAPTER_DESC1 Desc;
            Adapter->GetDesc1(&Desc);

            if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;

            if (SUCCEEDED(D3D12CreateDevice((IUnknown*)Adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device*), nullptr)))
                break;
        }
    }
    
    *RetAdapter = Adapter;
}

Device::Device()
{
    HRESULT Result = 0;

#ifdef ONI_DEBUG
    ID3D12Debug* debug;
    Result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
    if (FAILED(Result))
        Logger::Error("[D3D12] Failed to get debug interface!");

    debug->QueryInterface(IID_PPV_ARGS(&_debug));
    debug->Release();

    _debug->EnableDebugLayer();
    _debug->SetEnableGPUBasedValidation(true);
#endif

    Result = CreateDXGIFactory(IID_PPV_ARGS(&_factory));
    if (FAILED(Result))
        Logger::Error("[D3D12] Failed to create DXGI factory!");
    GetHardwareAdapter(_factory, &_adapter, true);

    Result = D3D12CreateDevice(_adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_device));
    if (FAILED(Result))
        Logger::Error("[D3D12] Failed to create device!");

#ifdef ONI_DEBUG
    Result = _device->QueryInterface(IID_PPV_ARGS(&_debugDevice));
    if (FAILED(Result))
        Logger::Error("[D3D12] Failed to query debug device!");

    ID3D12InfoQueue* InfoQueue = 0;
    _device->QueryInterface(IID_PPV_ARGS(&InfoQueue));

    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

    D3D12_MESSAGE_SEVERITY SupressSeverities[] = {
        D3D12_MESSAGE_SEVERITY_INFO
    };

    D3D12_MESSAGE_ID SupressIDs[] = {
        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
        D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
        D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
    };

    D3D12_INFO_QUEUE_FILTER filter = {0};
    filter.DenyList.NumSeverities = ARRAYSIZE(SupressSeverities);
    filter.DenyList.pSeverityList = SupressSeverities;
    filter.DenyList.NumIDs = ARRAYSIZE(SupressIDs);
    filter.DenyList.pIDList = SupressIDs;

    InfoQueue->PushStorageFilter(&filter);
    InfoQueue->Release();
#endif
    
    // Get hardware support
    _features.CheckSupport(_device);

    DXGI_ADAPTER_DESC Desc;
    _adapter->GetDesc(&Desc);
    std::wstring WideName = Desc.Description;
    std::string DeviceName = std::string(WideName.begin(), WideName.end());
    Logger::Info("[D3D12] Using GPU %s:", DeviceName.c_str());

    Logger::Info("[D3D12] Device Raytracing Support: %d", _features.Raytracing);
    Logger::Info("[D3D12] Device Mesh Shading Support: %d", _features.MeshShaders);
    Logger::Info("[D3D12] Device Work Graphs Support: %d", _features.WorkGraphs);
    Logger::Info("[D3D12] Available Video memory: %fgb", float(Desc.DedicatedVideoMemory / 1024.0f / 1024.0f / 1024.0f));
    Logger::Info("[D3D12] Available System memory: %fgb", float(Desc.DedicatedSystemMemory / 1024.0f / 1024.0f / 1024.0f));
    Logger::Info("[D3D12] Available Shared memory: %fgb", float(Desc.SharedSystemMemory / 1024.0f / 1024.0f / 1024.0f));

    _name = DeviceName;
}

Device::~Device()
{
    _device->Release();
    _factory->Release();
    _adapter->Release();
#if defined(ONI_DEBUG)
    _debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_IGNORE_INTERNAL | D3D12_RLDO_DETAIL);
    _debugDevice->Release();
    _debug->Release();
#endif
}

void DeviceFeatures::CheckSupport(ID3D12Device *device)
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 raytracingData = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 meshShaderData = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS21 workGraphData = {};

    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &raytracingData, sizeof(raytracingData));
    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &meshShaderData, sizeof(meshShaderData));
    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &workGraphData, sizeof(workGraphData));

    Raytracing = raytracingData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    MeshShaders = meshShaderData.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
    WorkGraphs = workGraphData.WorkGraphsTier != D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED;
}
