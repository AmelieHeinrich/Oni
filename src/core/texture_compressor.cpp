//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-14 09:39:03
//

#include <sstream>
#include <filesystem>
#include <stdlib.h>

#include "texture_compressor.hpp"
#include "util.hpp"
#include "file_system.hpp"
#include "log.hpp"
#include "texture_file.hpp"

class NVTTErrorHandler : nvtt::ErrorHandler
{
public:
     virtual void error(nvtt::Error e) override {
        switch (e) {
            case nvtt::Error::Error_UnsupportedOutputFormat: {
                Logger::Error("nvtt: Error_UnsupportedOutputFormat");
                break;
            }
            case nvtt::Error::Error_UnsupportedFeature: {
                Logger::Error("nvtt: Error_UnsupportedFeature");
                break;
            }
            case nvtt::Error::Error_Unknown: {
                Logger::Error("nvtt: Error_Unknown");
                break;
            }
            case nvtt::Error::Error_InvalidInput: {
                Logger::Error("nvtt: Error_InvalidInput");
                break;
            }
            case nvtt::Error::Error_FileWrite: {
                Logger::Error("nvtt: Error_FileWrite");
                break;
            }
            case nvtt::Error::Error_FileOpen: {
                Logger::Error("nvtt: Error_FileOpen");
                break;
            }
            case nvtt::Error::Error_CudaError: {
                Logger::Error("nvtt: Error_CudaError");
                break;
            }
            default: {
                Logger::Error("Unknown error!");
                break;
            }
        }
     }
};

class OniTextureFileWriter : nvtt::OutputHandler
{
public:
    OniTextureFileWriter(const std::string& path, int width, int height, int mipCount, int mode) {
        f = fopen(path.c_str(), "wb+");
        if (!f) {
            Logger::Error("Failed to fopen file %s", path.c_str());
        }

        TextureFile::Header header;
        header.width = width;
        header.height = height;
        header.mipCount = mipCount;
        header.mode = mode;
        
        fwrite(&header, sizeof(header), 1, f);
    }

    ~OniTextureFileWriter() {
        fclose(f);
    }

    virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) override {}
    virtual void endImage() override {}

    virtual bool writeData(const void * data, int size) override {
        fwrite(data, size, 1, f);
        return true;
    }

private:
    FILE *f;
};

void TextureCompressor::TraverseDirectory(const std::string& path, TextureCompressorFormat format)
{
    if (!FileSystem::Exists(".cache")) {
        FileSystem::CreateDirectoryFromPath(".cache/");
    }
    if (!FileSystem::Exists(".cache/textures/")) {
        FileSystem::CreateDirectoryFromPath(".cache/textures/");
    }

    NVTTErrorHandler errorHandler;

    nvtt::Context context;
    context.enableCudaAcceleration(true);
    
    if (context.isCudaAccelerationEnabled()) {
        Logger::Info("[TEXTURE CACHE] Thankfully for you, NVTT found a CUDA context! Enjoy the blazingly fast caching process.");
    } else {
        Logger::Info("[TEXTURE CACHE] No CUDA for you. Maybe update drivers, and if you have an AMD card... I'm sorry :(");
    }

    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat(nvtt::Format(format));

    int mode = format == TextureCompressorFormat::BC1 ? 1 : 7;

    for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(path)) {
        std::string entryPath = dirEntry.path().string();
        std::replace(entryPath.begin(), entryPath.end(), '\\', '/');
        
        std::string cached = GetCachedPath(entryPath);
        
        if (!IsValidExtension(FileSystem::GetFileExtension(entryPath))) {
            continue;
        }

        if (ExistsInCache(entryPath)) {
            Logger::Info("[TEXTURE CACHE] %s already compressed -- skipping.", entryPath.c_str());
            continue;
        }

        nvtt::Surface image;
        if (!image.load(entryPath.c_str())) {
            Logger::Error("nvtt: Failed to load texture");
        }

        int mipCount = image.countMipmaps();

        OniTextureFileWriter writer(cached, image.width(), image.height(), mipCount, mode);

        nvtt::OutputOptions outputOptions;
        outputOptions.setErrorHandler(reinterpret_cast<nvtt::ErrorHandler*>(&errorHandler));
        outputOptions.setOutputHandler(reinterpret_cast<nvtt::OutputHandler*>(&writer));

        for (int i = 0; i < mipCount; i++) {
            if (!context.compress(image, 0, i, compressionOptions, outputOptions)) {
                Logger::Error("Failed to compress texture!");
            }

            if (i == mipCount - 1) break;

            // Prepare the next mip:
            image.toLinearFromSrgb();
            image.premultiplyAlpha();

            image.buildNextMipmap(nvtt::MipmapFilter_Box);
        
            image.demultiplyAlpha();
            image.toSrgb();
        }

        Logger::Info("Compressed %s to %s", entryPath.c_str(), cached.c_str());
    }
}

bool TextureCompressor::ExistsInCache(const std::string& path)
{
    return FileSystem::Exists(GetCachedPath(path));
}

std::string TextureCompressor::GetCachedPath(const std::string& path)
{
    std::stringstream path_ss;

    const char *key = path.c_str();
    uint64_t hash = util::hash(key, path.length(), 1000);
    path_ss << ".cache/textures/" << hash << ".oni";

    return path_ss.str();
}

TextureFile TextureCompressor::GetFromCache(const std::string& path)
{
    // Sanitize in case
    if (!ExistsInCache(path)) {
        Logger::Error("Texture %s is uncached. Please restart Oni.", path.c_str());
    }
    
    std::string cached = GetCachedPath(path);
    Logger::Info("[TEXTURE CACHE] Getting texture %s (cached : %s)", path.c_str(), cached.c_str());
    return TextureFile(cached);
}

bool TextureCompressor::IsValidExtension(const std::string& extension)
{
    if (extension == ".png")
        return true;
    if (extension == ".jpg")
        return true;
    if (extension == ".jpeg")
        return true;

    return false;
}
