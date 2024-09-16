//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-16 21:44:36
//

#pragma once

#include <string>

// TODO(amelie): Make a file watcher

class Asset
{
public:
    Asset(const std::string& path)
        : _path(path)
    { 
    }
    virtual ~Asset() = default;

    virtual void OnReload(const std::string& path) = 0;
protected:
    std::string _path;
};
