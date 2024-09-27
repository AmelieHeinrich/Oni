//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-14 10:55:33
//

#include "texture_file.hpp"
#include "file_system.hpp"

TextureFile::TextureFile(const std::string& path)
{
    uint64_t size = FileSystem::GetFileSize(path);
    _byteSize = size - sizeof(Header);

    void *bytes = FileSystem::ReadBytes(path);
    memcpy(&_header, bytes, sizeof(Header));

    _bytes = malloc(_byteSize);

    char *pixels = (reinterpret_cast<char*>(bytes)) + sizeof(Header);
    memcpy(_bytes, pixels, _byteSize);

    delete bytes;
}

TextureFile::~TextureFile()
{
    delete _bytes;
}

Bitmap TextureFile::ToBitmap()
{
    Bitmap result = {};
    result.Width = _header.width;
    result.Height = _header.height;
    result.Mips = 1;
    result.HDR = false;
    result.Delete = false;
    result.Bytes = reinterpret_cast<char*>(malloc(_byteSize + 1));
    result.BufferSize = _byteSize;

    memcpy(result.Bytes, _bytes, _byteSize);

    return result;
}
