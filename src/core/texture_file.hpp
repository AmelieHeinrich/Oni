//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-14 10:48:46
//

#pragma once

#include <cstdint>

#include "bitmap.hpp"
#include "rhi/texture.hpp"

class TextureFile
{
public:
    struct Header
    {
        uint32_t width;
        uint32_t height;
        uint32_t mipCount;
        uint32_t mode;
    };

    TextureFile() {}
    TextureFile(const std::string& path);
    ~TextureFile();

    void Load(const std::string& path);

    uint32_t Width() { return _header.width; }
    uint32_t Height() { return _header.height; }
    uint32_t MipCount() { return _header.mipCount; }
    TextureFormat Format() { return _header.mode == 1 ? TextureFormat::BC1 : TextureFormat::BC7; }

    void *GetMipChainStart() { return _bytes; }

    Bitmap ToBitmap();
private:
    Header _header;
    void *_bytes = nullptr;
    uint64_t _byteSize;
};
