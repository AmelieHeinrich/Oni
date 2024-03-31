/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:53:11
 */

#define MAX_LIGHTS 512

struct FragmentIn
{
    float4 Position : SV_POSITION;
    float2 TexCoords : TEXCOORD; 
};

struct PointLight
{
    float4 Position;
    float4 Color;
};

struct LightData
{
    PointLight PointLights[MAX_LIGHTS];
    int PointLightCount;
    float3 Pad;
};

Texture2D Texture : register(t2);
Texture2D NormalTexture : register(t3);
SamplerState Sampler : register(s4);
ConstantBuffer<LightData> LightBuffer : register(b5);

float3 CalcPointLight(PointLight light, float3 normal, float3 position)
{
    float ambientStrength = 0.1f;
    float3 ambient = ambientStrength * light.Color.xyz;

    float3 lightDir = normalize(light.Position.xyz - position);
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * light.Color.xyz;

    return (ambient + diffuse);
}

float4 Main(FragmentIn Input) : SV_TARGET
{
    float4 albedo = Texture.Sample(Sampler, Input.TexCoords);
    float4 normal = normalize(NormalTexture.Sample(Sampler, Input.TexCoords));

    if (albedo.a < 0.25)
        discard;

    float3 FinalColor = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < LightBuffer.PointLightCount; i++) {
        FinalColor += CalcPointLight(LightBuffer.PointLights[i], normal.xyz, Input.Position.xyz);
    }

    return float4(FinalColor, 1.0) * albedo;
}