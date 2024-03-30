/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-02-18 12:22:17
 */

#pragma once

#include <string>

class FileSystem
{
public:
    static bool Exists(const std::string& path);
    static bool IsDirectory(const std::string& path);
    
    static void CreateFileFromPath(const std::string& path);
    static void CreateDirectoryFromPath(const std::string& path);

    static void Delete(const std::string& path);
    static void Move(const std::string& oldPath, const std::string& newPath);
    static void Copy(const std::string& oldPath, const std::string& newPath, bool overwrite = true);

    static int32_t GetFileSize(const std::string& path);
    static std::string ReadFile(const std::string& path);
    static void *ReadBytes(const std::string& path);
};
