/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 17:09:36
 */

#include "bitmap.hpp"

#include <core/log.hpp>
#include <stb/stb_image.h>

Bitmap::~Bitmap()
{
    if (Delete) {
        Destroy();
    }
}

void Bitmap::Destroy()
{
    if (Bytes != nullptr) {
        delete[] Bytes;
        Bytes = nullptr;
    }
}

void Bitmap::LoadFromFile(const std::string& path, bool flip)
{
    int channels = 0;

    stbi_set_flip_vertically_on_load(flip);
    Bytes = reinterpret_cast<char*>(stbi_load(path.c_str(), &Width, &Height, &channels, STBI_rgb_alpha));
    if (!Bytes) {
        Logger::Error("Failed to load image %s", path.c_str());
    } else {
        Logger::Info("Loaded texture: %s", path.c_str());
    }
}

void Bitmap::LoadHDR(const std::string& path)
{
    HDR = true;
    int channels;

    stbi_set_flip_vertically_on_load(false);
    Bytes = reinterpret_cast<char*>(stbi_load_16(path.c_str(), &Width, &Height, &channels, STBI_rgb_alpha));
    if (!Bytes) {
        Logger::Error("Failed to load image %s", path.c_str());
    } else {
        Logger::Info("Loaded HDR map: %s", path.c_str());
    }
}
