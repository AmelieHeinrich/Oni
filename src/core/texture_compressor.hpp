//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-14 09:36:58
//

#pragma once

#include <string>

#include "bitmap.hpp"
#include "texture_file.hpp"

#include <nvtt/nvtt.h>

enum class TextureCompressorFormat
{
    BC1 = nvtt::Format_BC1,
    BC7 = nvtt::Format_BC7
};

class TextureCompressor
{
public:
    // Compresses every texture file in given directory
    static void TraverseDirectory(const std::string& path, TextureCompressorFormat format);

    static bool ExistsInCache(const std::string& path);
    static TextureFile GetFromCache(const std::string& path);

    static std::string GetCachedPath(const std::string& path);

private:
    static bool IsValidExtension(const std::string& extension);
};
