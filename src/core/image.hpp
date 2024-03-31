/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 17:08:52
 */

#pragma once

#include <string>

struct Image
{
    char* Bytes = nullptr;
    int Width = -1;
    int Height = -1;
    bool Delete = true;

    ~Image();
    
    void Destroy();
    void LoadFromFile(const std::string& path, bool flip = true);
    void LoadHDR(const std::string& path);
};
