/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 22:16:27
 */

#pragma once

#include "device.hpp"
#include "descriptor_heap.hpp"
#include "command_queue.hpp"
#include "texture.hpp"

#define FRAMES_IN_FLIGHT 3

// TODO: Make the d-heap stuff global for convenience's sake

class SwapChain
{
public:
    using Ptr = std::shared_ptr<SwapChain>;

    SwapChain(Device::Ptr device, CommandQueue::Ptr graphicsQueue, DescriptorHeap::Ptr rtvHeap, HWND window);
    ~SwapChain();

    void Present(bool vsync);

    int AcquireImage();
    void Resize(uint32_t width, uint32_t height);
    Texture::Ptr GetTexture(uint32_t index) { return _textures[index]; }
private:
    Device::Ptr _devicePtr;
    DescriptorHeap::Ptr _rtvHeap;

    HWND _hwnd;
    IDXGISwapChain3* _swapchain;

    ID3D12Resource* _buffers[FRAMES_IN_FLIGHT];
    DescriptorHeap::Descriptor _descriptors[FRAMES_IN_FLIGHT];
    Texture::Ptr _textures[FRAMES_IN_FLIGHT];

    int _width;
    int _height;
};
