//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-13 14:26:27
//

#pragma once

#include <glm/gtc/type_ptr.hpp>

#define MAX_POINT_LIGHTS 512
#define MAX_DIRECTIONAL_LIGHTS 512

class PointLight
{
public:
    struct GPUData
    {
        glm::vec4 Position;
        glm::vec4 Color;
        float Brightness;
        uint32_t _pad0;
        uint32_t _pad1;
        uint32_t _pad2;
    };

public:
    PointLight() = default;
    PointLight(glm::vec3 position, glm::vec3 color, float brightness)
        : Position(position), Color(color), Brightness(brightness)
    {    
    }
    ~PointLight() = default;

    GPUData GetGPUData() {
        GPUData data = {};
        data.Position = glm::vec4(Position, 1.0);
        data.Color = glm::vec4(Color, 1.0);
        data.Brightness = Brightness;
        return data;
    }

    glm::vec3 Position;
    glm::vec3 Color;
    float Brightness;
};

class DirectionalLight
{
public:
    struct GPUData
    {
        glm::vec4 Direction;
        glm::vec4 Color;
    };

public:
    DirectionalLight() = default;
    DirectionalLight(glm::vec3 dir, glm::vec3 col)
        : Direction(dir), Color(col)
    {}
    ~DirectionalLight() = default;

    GPUData GetGPUData() {
        GPUData data = {};
        data.Direction = glm::vec4(Direction, 1.0);
        data.Color = glm::vec4(Color, 1.0);
        return data;
    }

    glm::vec3 Direction;
    glm::vec3 Color;
};

class LightSettings
{
public:
    struct GPUData
    {
        PointLight::GPUData PointLights[MAX_POINT_LIGHTS];
        int PointLightCount;
        glm::vec3 _Pad0;

        DirectionalLight::GPUData Sun;
        int HasSun;
        glm::vec3 _Pad1;
    };

public:
    LightSettings() = default;
    ~LightSettings() = default;

    GPUData GetGPUData() {
        GPUData data;
        data.HasSun = HasSun;
        data.Sun = Sun.GetGPUData();

        for (size_t i = 0; i < PointLights.size(); i++) {
            data.PointLights[i] = PointLights[i].GetGPUData();
            data.PointLightCount++;
        }

        return data;
    }

    void AddPointLight(PointLight light) {
        PointLights.push_back(light);
    }

    void SetSun(glm::vec3 position, glm::vec3 direction, glm::vec3 color) {
        HasSun = true;
        Sun.Direction = direction;
        Sun.Color = color;
    }

    std::vector<PointLight> PointLights;
    bool HasSun = false;
    DirectionalLight Sun;
    glm::vec3 SunPosition;
};
