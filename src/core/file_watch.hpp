//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-18 13:45:46
//

#pragma once

#include <string>
#include <Windows.h>

#include "timer.hpp"

class FileWatch
{
public:
    FileWatch() = default;
    FileWatch(const std::string& file);
    ~FileWatch();

    void Load(const std::string& file);
    bool Check();
private:
    std::string _file;
    Timer _checkTimer;
    HANDLE _handle;
    FILETIME _filetime;
};
