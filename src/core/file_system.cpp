/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-02-18 12:35:07
 */

#include "file_system.hpp"
#include "log.hpp"

#include <sys/stat.h>
#include <fstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

bool FileSystem::Exists(const std::string& path)
{
    struct stat statistics;
    if (stat(path.c_str(), &statistics) == -1)
        return false;
    return true;
}

bool FileSystem::IsDirectory(const std::string& path)
{
    struct stat statistics;
    if (stat(path.c_str(), &statistics) == -1)
        return false;
    return (statistics.st_mode & S_IFDIR) != 0;
}

void FileSystem::CreateFileFromPath(const std::string& path)
{
    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!handle) {
        Logger::Error("Error when creating file %s", path.c_str());
        return;
    }
    CloseHandle(handle);
}

void FileSystem::CreateDirectoryFromPath(const std::string& path)
{
    if (!CreateDirectoryA(path.c_str(), nullptr)) {
        Logger::Error("Error when creating directory %s", path.c_str());
    }
}

void FileSystem::Delete(const std::string& path)
{
    if (!Exists(path)) {
        Logger::Warn("Trying to delete file %s that doesn't exist!", path.c_str());
        return;
    }

    if (!DeleteFileA(path.c_str())) {
        Logger::Error("Failed to delete file %s", path.c_str());
    }
}

void FileSystem::Move(const std::string& oldPath, const std::string& newPath)
{
    if (!Exists(oldPath)) {
        Logger::Warn("Trying to move file %s that doesn't exist!", oldPath.c_str());
        return;
    }

    if (!MoveFileA(oldPath.c_str(), newPath.c_str())) {
        Logger::Error("Failed to move file %s to {1}", oldPath.c_str(), newPath.c_str());
    }
}

void FileSystem::Copy(const std::string& oldPath, const std::string& newPath, bool overwrite)
{
    if (!Exists(oldPath)) {
        Logger::Warn("Trying to copy file %s that doesn't exist!", oldPath.c_str());
        return;
    }

    if (!CopyFileA(oldPath.c_str(), newPath.c_str(), !overwrite)) {
        Logger::Error("Failed to copy file %s to {1}", oldPath.c_str(), newPath.c_str());
    }
}

int FileSystem::GetFileSize(const std::string& path)
{
    int result = 0;

    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!handle) {
        Logger::Error("File %s does not exist!", path.c_str());
        return 0;
    }
    result = ::GetFileSize(handle, nullptr);
    CloseHandle(handle);
    return result;
}

std::string FileSystem::ReadFile(const std::string& path)
{
    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!handle)
    {
        Logger::Error("File %s does not exist and cannot be read!", path);
        return std::string("");
    }
    int size = FileSystem::GetFileSize(path);
    if (size == 0)
    {
        Logger::Error("File %s has a size of 0, thus cannot be read!", path);
        return std::string("");
    }
    int bytesRead = 0;
    char *buffer = new char[size + 1];
    ::ReadFile(handle, reinterpret_cast<LPVOID>(buffer), size, reinterpret_cast<LPDWORD>(&bytesRead), nullptr);
    buffer[size] = '\0';
    CloseHandle(handle);
    return std::string(buffer);
}

void *FileSystem::ReadBytes(const std::string& path)
{
    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!handle)
    {
        Logger::Error("File %s does not exist and cannot be read!", path);
        return nullptr;
    }
    int size = FileSystem::GetFileSize(path);
    if (size == 0)
    {
        Logger::Error("File %s has a size of 0, thus cannot be read!", path);
        return nullptr;
    }
    int bytesRead = 0;
    char *buffer = new char[size + 1];
    ::ReadFile(handle, reinterpret_cast<LPVOID>(buffer), size, reinterpret_cast<LPDWORD>(&bytesRead), nullptr);
    CloseHandle(handle);
    return buffer;
}
