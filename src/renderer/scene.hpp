/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:35:56
 */

#pragma once

#include "model.hpp"
#include "camera.hpp"

#include <glm/gtc/type_ptr.hpp>

#define MAX_POINT_LIGHTS 512

struct PointLight
{
    glm::vec4 Position;
    glm::vec4 Color;
    float Brightness;
    uint32_t _pad0;
    uint32_t _pad1;
    uint32_t _pad2;
};

struct DirectionalLight
{
    glm::vec4 Position;
    glm::vec4 Direction;
    glm::vec4 Color;
};

struct LightData
{
    PointLight PointLights[MAX_POINT_LIGHTS];
    int PointLightCount;
    glm::vec3 _Pad0;

    DirectionalLight Sun;
    int HasSun;
    glm::vec3 _Pad1;
};

struct Scene
{
    std::vector<Model> Models;

    FreeCamera Camera;
    
    LightData LightBuffer;
};
