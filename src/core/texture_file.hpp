//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-14 10:48:46
//

#pragma once

#include <cstdint>

#include "bitmap.hpp"

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

    TextureFile(const std::string& path);
    ~TextureFile();

    Bitmap ToBitmap();
private:
    Header _header;
    void *_bytes;
    uint64_t _byteSize;
};
