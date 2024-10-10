/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 16:21:27
 */

#pragma once

#include <vector>

#include "command_queue.hpp"
#include "command_buffer.hpp"
#include "device.hpp"

#include "raytracing/blas.hpp"
#include "raytracing/tlas.hpp"

#include "core/bitmap.hpp"
#include "core/texture_file.hpp"

class Uploader
{
public:
    Uploader(Device::Ptr device, Allocator::Ptr allocator, DescriptorHeap::Heaps& heaps);
    ~Uploader();

    void CopyHostToDeviceShared(void* pData, uint64_t uiSize, Buffer::Ptr pDestBuffer);
    void CopyHostToDeviceLocal(void* pData, uint64_t uiSize, Buffer::Ptr pDestBuffer);
    void CopyHostToDeviceTexture(Bitmap& image, Texture::Ptr pDestTexture);
    void CopyHostToDeviceCompressedTexture(TextureFile *file, Texture::Ptr pDestTexture);
    void CopyBufferToBuffer(Buffer::Ptr pSourceBuffer, Buffer::Ptr pDestBuffer);
    void CopyTextureToTexture(Texture::Ptr pSourceTexture, Texture::Ptr pDestTexture);
    void CopyBufferToTexture(Buffer::Ptr pSourceBuffer, Texture::Ptr pDestTexture);
    void CopyTextureToBuffer(Texture::Ptr pSourceTexture, Buffer::Ptr pDestBuffer);
    
    void BuildBLAS(BLAS::Ptr blas);
    void BuildTLAS(TLAS::Ptr tlas);
private:
    friend class RenderContext;

    Device::Ptr _devicePtr;
    Allocator::Ptr _allocator;
    CommandBuffer::Ptr _commandBuffer;
    DescriptorHeap::Heaps _heaps;

private:
    enum class UploadCommandType
    {
        HostToDeviceShared,
        HostToDeviceLocal,
        HostToDeviceLocalTexture,
        HostToDeviceCompressedTexture,
        BufferToBuffer,
        TextureToTexture,
        BufferToTexture,
        TextureToBuffer,
        BuildBLAS,
        BuildTLAS
    };

    struct UploadCommand
    {
        UploadCommandType type;
        void* data;
        uint64_t size;

        TextureFile *textureFile;

        Texture::Ptr sourceTexture;
        Texture::Ptr destTexture;

        Buffer::Ptr sourceBuffer;
        Buffer::Ptr destBuffer;

        BLAS::Ptr blas;
        TLAS::Ptr tlas;
    };
    std::vector<UploadCommand> _commands;
};
