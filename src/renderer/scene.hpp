/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:35:56
 */

#pragma once

#include "model.hpp"

#include <glm/gtc/type_ptr.hpp>

#define MAX_POINT_LIGHTS 512

struct PointLight
{
    glm::vec4 Position;
    glm::vec4 Color;
};

struct LightData
{
    PointLight PointLights[MAX_POINT_LIGHTS];
    int PointLightCount;
    glm::vec3 Pad;
};

struct Scene
{
    std::vector<Model> Models;
    
    glm::mat4 View;
    glm::mat4 Projection;
    
    LightData LightBuffer;
};
