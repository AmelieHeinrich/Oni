/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 16:25:42
 */

#include "uploader.hpp"

#include "core/log.hpp"

Uploader::Uploader(Device::Ptr device, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps)
    : _devicePtr(device), _allocator(allocator), _heaps(heaps)
{
    _commandBuffer = std::make_shared<CommandBuffer>(device, allocator, heaps, CommandQueueType::Graphics, false);
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
    uint32_t numMips = file->MipCount();
    
    D3D12_RESOURCE_DESC desc = pDestTexture->GetResource().Resource->GetDesc();

    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(numMips);
    std::vector<uint32_t> numRows(numMips);
    std::vector<uint64_t> rowSizes(numMips);
    uint64_t totalSize = 0;

    _devicePtr->GetDevice()->GetCopyableFootprints(&desc, 0, numMips, 0, footprints.data(), numRows.data(), rowSizes.data(), &totalSize);

    Buffer::Ptr buf = std::make_shared<Buffer>(_devicePtr, _allocator, _heaps, totalSize, 0, BufferType::Copy, false, "Staging Buffer");
    
    uint8_t *pixels = reinterpret_cast<uint8_t*>(file->GetMipChainStart());
    uint8_t *pData;

    buf->Map(0, 0, reinterpret_cast<void**>(&pData));
    memset(pData, 0, totalSize);
    for (int i = 0; i < numMips; i++) {
        for (int j = 0; j < numRows[i]; j++) {
            memcpy(pData, pixels, rowSizes[i]);

            pData += footprints[i].Footprint.RowPitch;
            pixels += rowSizes[i];
        }
    }
    buf->Unmap(0, 0);

    UploadCommand command;
    command.type = UploadCommandType::HostToDeviceCompressedTexture;
    command.textureFile = file;
    command.destTexture = pDestTexture;
    command.sourceBuffer = buf;

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
