//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-19 15:18:16
//

#define MAX_LIGHTS 512

struct PointLight
{
    float4 Position;
    float4 Color;
    float Brightness;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

struct DirectionalLight
{
    float4 Direction;
    float4 Color;
};

struct LightData
{
    PointLight PointLights[MAX_LIGHTS];
    int PointLightCount;
    float3 _Pad0;

    DirectionalLight Sun;
    int HasSun;
    float3 _Pad1;
};
