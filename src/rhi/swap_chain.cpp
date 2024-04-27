/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:19:59
 */

#include "swap_chain.hpp"

#include <core/log.hpp>
#include <optick.h>

SwapChain::SwapChain(Device::Ptr device, CommandQueue::Ptr graphicsQueue, DescriptorHeap::Ptr rtvHeap, HWND window)
    : _devicePtr(device), _rtvHeap(rtvHeap), _hwnd(window)
{
    RECT ClientRect;
    GetClientRect(_hwnd, &ClientRect);
    _width = ClientRect.right - ClientRect.left;
    _height = ClientRect.bottom - ClientRect.top;

    DXGI_SWAP_CHAIN_DESC1 Desc = {};
    Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Desc.SampleDesc.Count = 1;
    Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    Desc.BufferCount = FRAMES_IN_FLIGHT;
    Desc.Scaling = DXGI_SCALING_NONE;
    Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    Desc.Width = _width;
    Desc.Height = _height;

    IDXGISwapChain1* temp;
    HRESULT result = device->GetFactory()->CreateSwapChainForHwnd(graphicsQueue->GetQueue(), _hwnd, &Desc, nullptr, nullptr, &temp);
    if (FAILED(result)) {
        Logger::Error("D3D12: Failed to create swap chain!");
    }
    temp->QueryInterface(IID_PPV_ARGS(&_swapchain));
    temp->Release();

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&_buffers[i]));
        if (FAILED(result)) {
            Logger::Error("D3D12: Failed to get swapchain backbuffer at index %d", i);
        }

        _descriptors[i] = _rtvHeap->Allocate();
        _devicePtr->GetDevice()->CreateRenderTargetView(_buffers[i], nullptr, _descriptors[i].CPU);

        _textures[i] = std::make_shared<Texture>(_devicePtr);
        _textures[i]->_release = false;
        _textures[i]->_resource = new GPUResource;
        _textures[i]->_resource->Resource = _buffers[i];
        _textures[i]->_rtv = _descriptors[i];
        _textures[i]->_format = TextureFormat::RGBA8;
        _textures[i]->_mipLevels = 1;
        _textures[i]->_states.push_back(D3D12_RESOURCE_STATE_COMMON);
    }
}

SwapChain::~SwapChain()
{
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        _buffers[i]->Release();
        _rtvHeap->Free(_descriptors[i]);
        delete _textures[i]->_resource;
    }
    _swapchain->Release();
}

void SwapChain::Present(bool vsync)
{
    OPTICK_EVENT("Present");

    HRESULT result = _swapchain->Present(vsync, 0);
    if (FAILED(result))
        Logger::Error("D3D12: Failed to present swapchain!");
}

int SwapChain::AcquireImage()
{
    OPTICK_EVENT("Acquire Image");

    return _swapchain->GetCurrentBackBufferIndex();
}

void SwapChain::Resize(uint32_t width, uint32_t height)
{
    OPTICK_EVENT("Resize");

    _width = width;
    _height = height;

    if (_swapchain) {
        for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
            _buffers[i]->Release();
            _rtvHeap->Free(_descriptors[i]);
            delete _textures[i]->_resource;
            _textures[i].reset();
        }

        HRESULT result = _swapchain->ResizeBuffers(0, _width, _height, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(result)) {
            Logger::Error("D3D12: Failed to resize swapchain!");
        }

        for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
            result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&_buffers[i]));
            if (FAILED(result)) {
                Logger::Error("D3D12: Failed to get swapchain backbuffer at index %d", i);
            }
    
            _descriptors[i] = _rtvHeap->Allocate();
            _devicePtr->GetDevice()->CreateRenderTargetView(_buffers[i], nullptr, _descriptors[i].CPU);

            _textures[i] = std::make_shared<Texture>(_devicePtr);
            _textures[i]->_release = false;
            _textures[i]->_resource = new GPUResource;
            _textures[i]->_resource->Resource = _buffers[i];
            _textures[i]->_rtv = _descriptors[i];
            _textures[i]->_format = TextureFormat::RGBA8;
            _textures[i]->_mipLevels = 1;
            _textures[i]->_states.push_back(D3D12_RESOURCE_STATE_COMMON);
        }
    }
}
