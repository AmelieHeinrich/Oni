//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-29 15:30:15
//

#include "shader_loader.hpp"

#include <sstream>
#include <filesystem>
#include <stdlib.h>
#include <algorithm>

#include "util.hpp"
#include "file_system.hpp"
#include "log.hpp"

void ShaderLoader::TraverseDirectory(const std::string& path)
{
    if (!FileSystem::Exists(".cache")) {
        FileSystem::CreateDirectoryFromPath(".cache/");
    }
    if (!FileSystem::Exists(".cache/shaders")) {
        FileSystem::CreateDirectoryFromPath(".cache/shaders/");
    }

    for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(path)) {
        if (dirEntry.is_directory()) {
            continue;
        }

        std::string entryPath = dirEntry.path().string();
        std::string cached = GetCachedPath(entryPath);

        std::replace(entryPath.begin(), entryPath.end(), '\\', '/');

        if (GetTypeFromPath(entryPath) == ShaderType::None) {
            continue;
        }

        if (ExistsInCache(entryPath)) {
            if (!ShouldReCache(entryPath)) {
                Logger::Info("[SHADER CACHE] %s is already cached -- skipping.", entryPath.c_str());
                continue;
            }
        }
        
        CacheShader(entryPath);
    }
}

bool ShaderLoader::ExistsInCache(const std::string& path)
{
    return FileSystem::Exists(GetCachedPath(path));
}

ShaderBytecode ShaderLoader::GetFromCache(const std::string& path)
{
    ShaderBytecode bytecode;

    if (!ExistsInCache(path)) {
        CacheShader(path);
    } else {
        OniShaderHeader header;

        // Read header
        FILE* f = fopen(GetCachedPath(path).c_str(), "rb");
        fread(&header, sizeof(header), 1, f);

        // Read bytecode
        bytecode.bytecode = std::vector<uint32_t>(header.BytecodeSize);
        fread(bytecode.bytecode.data(), bytecode.bytecode.size() * sizeof(uint32_t), 1, f);

        fclose(f);
    }

    return bytecode;
}

std::string ShaderLoader::GetCachedPath(const std::string& path)
{
    std::stringstream path_ss;

    const char *key = path.c_str();
    uint64_t hash = util::hash(key, path.length(), 1000);
    path_ss << ".cache/shaders/" << hash << ".oni";

    return path_ss.str();
}

ShaderType ShaderLoader::GetTypeFromPath(const std::string& path)
{
    // Special case, in case :p
    if (path.find("shaders/Common/Compute.hlsl") != std::string::npos) {
        return ShaderType::None;
    }

    if (path.find("Vert") != std::string::npos) {
        return ShaderType::Vertex;
    }
    if (path.find("Frag") != std::string::npos) {
        return ShaderType::Fragment;
    }
    if (path.find("Compute") != std::string::npos) {
        return ShaderType::Compute;
    }
    return ShaderType::None;
}

bool ShaderLoader::ShouldReCache(const std::string& path)
{
    if (!ExistsInCache(path)) {
        return true;
    }

    // Get file write of uncached shader
    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!handle) {
        Logger::Info("What the fuck? 1");
        return true;
    }
    FILETIME temp;
    GetFileTime(handle, nullptr, nullptr, &temp);
    CloseHandle(handle);

    // Get shader header
    std::string cached = GetCachedPath(path);
    FILE* f = fopen(cached.c_str(), "rb");
    if (!f) {
        Logger::Error("What the fuck? 2");
        return true;
    }

    OniShaderHeader header;
    fread(&header, sizeof(header), 1, f);
    fclose(f);

    // Check if file times are different
    if (temp.dwLowDateTime != header.LowFileTime || temp.dwHighDateTime != header.HighFileTime) {
        return true;
    }
    return false;
}

void ShaderLoader::CacheShader(const std::string& path)
{
    std::string cached = GetCachedPath(path);

    ShaderType type = GetTypeFromPath(path);
    ShaderBytecode bytecode;

    ShaderCompiler::CompileShader(path, "Main", type, bytecode);

    // Get file time
    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    FILETIME temp;
    GetFileTime(handle, nullptr, nullptr, &temp);
    CloseHandle(handle);

    OniShaderHeader header;
    header.Type = type;
    header.LowFileTime = temp.dwLowDateTime;
    header.HighFileTime = temp.dwHighDateTime;
    header.BytecodeSize = bytecode.bytecode.size();

    FILE* f = fopen(cached.c_str(), "wb+");
    fwrite(&header, sizeof(header), 1, f);
    fwrite(bytecode.bytecode.data(), bytecode.bytecode.size() * sizeof(uint32_t), 1, f);
    fclose(f);

    Logger::Info("Cached shader %s in %s", path.c_str(), cached.c_str());
}
