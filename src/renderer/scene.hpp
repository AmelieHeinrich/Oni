/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:35:56
 */

#pragma once

#include <vector>

#include "lights.hpp"

#include "core/model.hpp"
#include "core/camera.hpp"

struct Scene
{
    Scene() = default;
    ~Scene() = default;

    void Update();

    FreeCamera Camera = FreeCamera();
    glm::mat4 PrevViewProj = glm::mat4(1.0f); // For velocity buffer

    std::vector<Model> Models;
    LightSettings Lights;
};
