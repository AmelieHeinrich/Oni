/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:35:56
 */

#pragma once

#include <vector>

#include "model.hpp"
#include "camera.hpp"
#include "lights.hpp"

struct Scene
{
    Scene() = default;
    ~Scene() = default;

    void Update();

    FreeCamera Camera = FreeCamera();

    std::vector<Model> Models;
    LightSettings Lights;
};
