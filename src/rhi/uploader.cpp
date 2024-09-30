/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 16:25:42
 */

#include "uploader.hpp"

Uploader::Uploader(Device::Ptr device, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps)
    : _devicePtr(device), _allocator(allocator), _heaps(heaps)
{
    _commandBuffer = std::make_shared<CommandBuffer>(device, heaps, CommandQueueType::Graphics, false);
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

void Uploader::CopyHostToDeviceTexture(Bitmap& image, Texture::Ptr pDestTexture)
{
    int componentSize = Texture::GetComponentSize(pDestTexture->GetFormat());
    int bufferSize = image.Width * image.Height * componentSize;
    if (image.BufferSize != 0) {
        bufferSize = image.BufferSize;
    }

    Buffer::Ptr buffer = std::make_shared<Buffer>(_devicePtr, _allocator, _heaps, bufferSize, 0, BufferType::Copy, false);

    {
        UploadCommand command;
        command.type = UploadCommandType::HostToDeviceShared;
        command.data = image.Bytes;
        command.size = bufferSize;
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

void Uploader::CopyHostToDeviceCompressedTexture(TextureFile *file, Texture::Ptr pDestTexture)
{
    int textureSize = file->Width();
    int mipCount = file->MipCount();

    UploadCommand command;
    command.type = UploadCommandType::HostToDeviceCompressedTexture;
    command.textureFile = file;
    command.destTexture = pDestTexture;

    command.mipBuffers.resize(mipCount);
    for (int level = 0; level < mipCount; ++level) {
        int bufferSize = command.textureFile->GetMipByteSize(level);
        command.mipBuffers[level] = std::make_shared<Buffer>(_devicePtr, _allocator, _heaps, bufferSize, 0, BufferType::Copy, false);

        void *pData;
        command.mipBuffers[level]->Map(0, 0, &pData);
        memcpy(pData, command.textureFile->GetTexelsAtMip(level), bufferSize);
        command.mipBuffers[level]->Unmap(0, 0);
    }

    _commands.push_back(command);
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
