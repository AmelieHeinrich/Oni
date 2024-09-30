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

    _bytes = malloc(_byteSize);

    FILE* f = fopen(path.c_str(), "rb+");
    fread(&_header, sizeof(_header), 1, f);
    fread(_bytes, _byteSize, 1, f);
    fclose(f);
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
    result.Mips = _header.mipCount;
    result.HDR = false;
    result.Delete = false;
    result.Bytes = reinterpret_cast<char*>(malloc(_byteSize + 1));
    result.BufferSize = _byteSize;

    memcpy(result.Bytes, _bytes, _byteSize);

    return result;
}

void *TextureFile::GetTexelsAtMip(int level)
{
    if (level == 0) {
        return (_bytes);
    }

    int offset = 0;
    for (int i = 1; i < level; i++) {
        offset += GetMipByteSize(level - 1);
    }

    return (void*)((char*)_bytes + offset); 
}

uint32_t TextureFile::GetMipDimension(int level)
{
    if (level == 0) {
        return _header.width;
    }

    return (_header.width / (level * 2));
}

uint64_t TextureFile::GetMipByteSize(int level)
{
    int dimension = GetMipDimension(level);
    return (dimension * dimension * Texture::GetComponentSize(Format()));
}
