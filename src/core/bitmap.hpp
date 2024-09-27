/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 17:08:52
 */

#pragma once

#include <string>

// TODO: rework

struct Bitmap
{
    char* Bytes = nullptr;
    int Width = -1;
    int Height = -1;
    int BufferSize = 0;
    uint32_t Mips = 1;
    bool Delete = true;
    bool HDR = false;

    ~Bitmap();
    
    void Destroy();
    void LoadFromFile(const std::string& path, bool flip = true);
    void LoadHDR(const std::string& path);
};
