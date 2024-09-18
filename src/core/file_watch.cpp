//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-18 13:48:58
//

#include "file_watch.hpp"
#include "log.hpp"

FileWatch::FileWatch(const std::string& file)
{
    _file = file;
   Load(file);
}

FileWatch::~FileWatch()
{
    //CloseHandle(_handle);
}

void FileWatch::Load(const std::string& file)
{
    _file = file;
    _handle = CreateFileA(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!_handle) {
        Logger::Error("Failed to start file watch on path %s", file.c_str());
    }

    GetFileTime(_handle, nullptr, nullptr, &_filetime);
    CloseHandle(_handle);
}

bool FileWatch::Check()
{
    if (_checkTimer.GetElapsed() >= 500.0f) {
        _checkTimer.Restart();
        _handle = CreateFileA(_file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (!_handle) {
            return false;
        }
        FILETIME temp;
        GetFileTime(_handle, nullptr, nullptr, &temp);
        CloseHandle(_handle);

        if (_filetime.dwLowDateTime != temp.dwLowDateTime || _filetime.dwHighDateTime != temp.dwHighDateTime) {
            _filetime = temp;
            return true;
        }
        return false;
    }
    return false;
}
