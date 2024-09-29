//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-29 15:29:00
//

#pragma once

#include <string>
#include <Windows.h>

#include "shader_bytecode.hpp"

struct OniShaderHeader
{
    ShaderType Type;
    DWORD LowFileTime;
    DWORD HighFileTime;
    uint32_t BytecodeSize;
};

class ShaderLoader
{
public:
    static void TraverseDirectory(const std::string& path);

    static bool ExistsInCache(const std::string& path);
    static ShaderBytecode GetFromCache(const std::string& path);

    static std::string GetCachedPath(const std::string& path);

private:
    static bool ShouldReCache(const std::string& path);
    static void CacheShader(const std::string& path);
    static ShaderType GetTypeFromPath(const std::string& path);
};
