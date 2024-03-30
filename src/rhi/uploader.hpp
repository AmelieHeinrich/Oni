/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 16:21:27
 */

#pragma once

#include "command_queue.hpp"
#include "command_buffer.hpp"
#include "device.hpp"

#include <vector>

class Uploader
{
public:
    Uploader(Device::Ptr device, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps);
    ~Uploader();

    void CopyHostToDeviceShared(void* pData, uint64_t uiSize, Buffer::Ptr pDestBuffer);
    void CopyHostToDeviceLocal(void* pData, uint64_t uiSize, Buffer::Ptr pDestBuffer);
    void CopyBufferToBuffer(Buffer::Ptr pSourceBuffer, Buffer::Ptr pDestBuffer);
    void CopyTextureToTexture(Texture::Ptr pSourceTexture, Texture::Ptr pDestTexture);
private:
    friend class RenderContext;

    Device::Ptr _devicePtr;
    Allocator::Ptr _allocator;
    CommandBuffer::Ptr _commandBuffer;

private:
    enum class UploadCommandType
    {
        HostToDeviceShared,
        HostToDeviceLocal,
        BufferToBuffer,
        TextureToTexture
    };

    struct UploadCommand
    {
        UploadCommandType type;
        void* data;
        uint64_t size;

        Texture::Ptr sourceTexture;
        Texture::Ptr destTexture;

        Buffer::Ptr sourceBuffer;
        Buffer::Ptr destBuffer;
    };
    std::vector<UploadCommand> _commands;
};
