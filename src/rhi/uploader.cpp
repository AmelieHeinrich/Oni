/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 16:25:42
 */

#include "uploader.hpp"

Uploader::Uploader(Device::Ptr device, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps)
    : _devicePtr(device), _allocator(allocator), _heaps(heaps)
{
    _commandBuffer = std::make_shared<CommandBuffer>(device, heaps, CommandQueueType::Graphics);
}

Uploader::~Uploader()
{
}

void Uploader::CopyHostToDeviceShared(void* pData, uint64_t uiSize, Buffer::Ptr pDestBuffer)
{
    UploadCommand command;
    command.type = UploadCommandType::HostToDeviceShared;
    command.data = pData;
    command.size = uiSize;
    command.destBuffer = pDestBuffer;

    _commands.push_back(command);
}

void Uploader::CopyHostToDeviceLocal(void* pData, uint64_t uiSize, Buffer::Ptr pDestBuffer)
{
    Buffer::Ptr buffer = std::make_shared<Buffer>(_devicePtr, _allocator, _heaps, uiSize, 0, BufferType::Copy, false);

    {
        UploadCommand command;
        command.type = UploadCommandType::HostToDeviceShared;
        command.data = pData;
        command.size = uiSize;
        command.destBuffer = buffer;

        _commands.push_back(command);
    }
    {
        UploadCommand command;
        command.type = UploadCommandType::HostToDeviceLocal;
        command.sourceBuffer = buffer;
        command.destBuffer = pDestBuffer;

        _commands.push_back(command);
    }
}

void Uploader::CopyHostToDeviceTexture(Image& image, Texture::Ptr pDestTexture)
{
    int componentSize = Texture::GetComponentSize(pDestTexture->GetFormat());
    Buffer::Ptr buffer = std::make_shared<Buffer>(_devicePtr, _allocator, _heaps, image.Width * image.Height * componentSize, 0, BufferType::Copy, false);

    {
        UploadCommand command;
        command.type = UploadCommandType::HostToDeviceShared;
        command.data = image.Bytes;
        command.size = image.Width * image.Height * componentSize;
        command.destBuffer = buffer;

        _commands.push_back(command);
    }
    {
        UploadCommand command;
        command.type = UploadCommandType::BufferToTexture;
        command.sourceBuffer = buffer;
        command.destTexture = pDestTexture;

        _commands.push_back(command);
    }
}

void Uploader::CopyBufferToBuffer(Buffer::Ptr pSourceBuffer, Buffer::Ptr pDestBuffer)
{
    UploadCommand command;
    command.type = UploadCommandType::BufferToBuffer;
    command.sourceBuffer = pSourceBuffer;
    command.destBuffer = pDestBuffer;
    
    _commands.push_back(command);
}

void Uploader::CopyTextureToTexture(Texture::Ptr pSourceTexture, Texture::Ptr pDestTexture)
{
    UploadCommand command;
    command.type = UploadCommandType::TextureToTexture;
    command.sourceTexture = pSourceTexture;
    command.destTexture = pDestTexture;
    
    _commands.push_back(command);
}

void Uploader::CopyBufferToTexture(Buffer::Ptr pSourceBuffer, Texture::Ptr pDestTexture)
{
    UploadCommand command;
    command.type = UploadCommandType::BufferToTexture;
    command.sourceBuffer = pSourceBuffer;
    command.destTexture = pDestTexture;

    _commands.push_back(command);
}

void Uploader::CopyTextureToBuffer(Texture::Ptr pSourceTexture, Buffer::Ptr pDestBuffer)
{
    UploadCommand command;
    command.type = UploadCommandType::TextureToBuffer;
    command.sourceTexture = pSourceTexture;
    command.destBuffer = pDestBuffer;

    _commands.push_back(command);
}
