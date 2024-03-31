/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 17:09:36
 */

#include "image.hpp"

#include <core/log.hpp>
#include <stb/stb_image.h>

Image::~Image()
{
    Destroy();
}

void Image::Destroy()
{
    if (Bytes != nullptr) {
        delete[] Bytes;
        Bytes = nullptr;
    }
}

void Image::LoadFromFile(const std::string& path, bool flip)
{
    int channels;

    stbi_set_flip_vertically_on_load(flip);
    Bytes = reinterpret_cast<char*>(stbi_load(path.c_str(), &Width, &Height, &channels, STBI_rgb_alpha));
    if (!Bytes) {
        Logger::Error("Failed to load image %s", path.c_str());
    } else {
        Logger::Info("Loaded texture: %s", path.c_str());
    }
}

void Image::LoadHDR(const std::string& path)
{
    int channels;

    stbi_set_flip_vertically_on_load(false);
    Bytes = reinterpret_cast<char*>(stbi_loadf(path.c_str(), &Width, &Height, &channels, STBI_rgb_alpha));
    if (!Bytes) {
        Logger::Error("Failed to load image %s", path.c_str());
    }
}
