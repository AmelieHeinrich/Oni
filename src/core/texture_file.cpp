//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-14 10:55:33
//

#include "texture_file.hpp"
#include "file_system.hpp"
#include "log.hpp"

TextureFile::TextureFile(const std::string& path)
{
    Load(path);
}

TextureFile::~TextureFile()
{
    if (_bytes != nullptr) {
        free(_bytes);
    }
}

void TextureFile::Load(const std::string& path)
{
    if (!FileSystem::Exists(path)) {
        Logger::Error("[TEXTURE FILE] %s doesn't exist!", path.c_str());
    }

    uint64_t size = FileSystem::GetFileSize(path);
    _byteSize = size - sizeof(Header);

    _bytes = malloc(_byteSize);

    // used to inspect the raw string in remedybg
    const char* debugString = path.c_str();
    FILE* f = fopen(debugString, "rb+");
    fread(&_header, sizeof(_header), 1, f);
    fread(_bytes, _byteSize, 1, f);
    fclose(f);
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
