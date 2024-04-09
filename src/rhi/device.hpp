/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 18:14:13
 */

#pragma once

#include <agility/d3d12.h>
#include <dxgi1_6.h>

#include <cstdint>
#include <memory>
#include <string>

struct DeviceFeatures {
    bool Raytracing = false;
    bool MeshShaders = false;
    bool WorkGraphs = false;
    bool IsComplete() {
        return Raytracing && MeshShaders && WorkGraphs;
    }
    void CheckSupport(ID3D12Device *device);
};

class Device
{
public:
    using Ptr = std::shared_ptr<Device>;

    Device();
    ~Device();

    ID3D12Device* GetDevice() { return _device; }
    IDXGIFactory3* GetFactory() { return _factory; }
    IDXGIAdapter1* GetAdapter() { return _adapter; }

    std::string GetName() { return _name; }
    DeviceFeatures GetFeatures() { return _features; }
private:
    std::string _name;
    DeviceFeatures _features;

    ID3D12Device* _device;
    ID3D12Debug1* _debug;
    ID3D12DebugDevice* _debugDevice;
    IDXGIAdapter1* _adapter;
    IDXGIDevice* _dxgiDevice;
    IDXGIFactory3* _factory;
};
