/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-29 13:02:08
 */

#pragma once

#include "buffer.hpp"
#include "texture.hpp"

enum class CommandQueueType;

class CommandBuffer
{
public:
    using Ptr = std::shared_ptr<CommandBuffer>;

    CommandBuffer(Device::Ptr devicePtr, CommandQueueType type);
    ~CommandBuffer();

    void Begin();
    void End();

    void ImageBarrier(Texture::Ptr texture, TextureLayout newLayout);

    ID3D12GraphicsCommandList* GetCommandList() { return _commandList; }
private:
    D3D12_COMMAND_LIST_TYPE _type;
    ID3D12GraphicsCommandList* _commandList; // TODO(ahi): Switch to newer version of command list to get access to DXR, Mesh shaders and Work Graphs
    ID3D12CommandAllocator* _commandAllocator;
};
