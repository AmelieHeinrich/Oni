//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-19 15:01:31
//

#include "shaders/Common/Math.hlsl"
#include "shaders/Common/Lights.hlsl"
#include "shaders/Common/PBR.hlsl"

#define MODE_DEFAULT 0
#define MODE_ALBEDO 1
#define MODE_NORMAL 2
#define MODE_MR 3
#define MODE_AO 4
#define MODE_EMISSIVE 5
#define MODE_SPECULAR 6
#define MODE_AMBIENT 7
#define MODE_POSITION 8

struct SceneData
{
    column_major float4x4 CameraMatrix;
    column_major float4x4 CameraProjViewInv;
    column_major float4x4 SunMatrix;
    column_major float4x4 SunView;
    float4 CameraPosition;
};

struct OutputBuffer
{
    int Mode;
    bool IBL;
    uint _Padding1;
    uint _Padding2;
};

struct FragmentIn
{
    float4 Position : SV_POSITION;
    float2 TexCoords : TEXCOORD;
};

struct FragmentData
{
    float3 Normals;
    float4 WorldPos;
    float4 LightPos;
};

Texture2D DepthBuffer : register(t0);
Texture2D Normals : register(t1);
Texture2D AlbedoEmissive : register(t2);
Texture2D PbrAO : register(t3);

TextureCube Irradiance : register(t4);
TextureCube Prefilter : register(t5);
Texture2D BRDF : register(t6);
Texture2D ShadowMap : register(t7);

SamplerState Sampler : register(s8);
SamplerComparisonState ShadowSampler : register(s9);

ConstantBuffer<SceneData> SceneBuffer : register(b10);
ConstantBuffer<LightData> LightBuffer : register(b11);
ConstantBuffer<OutputBuffer> OutputData : register(b12);

float3 CalcPointLight(FragmentData Input, PointLight light, float3 V, float3 N, float3 F0, float roughness, float metallic, float4 albedo)
{
    float3 lightPos = light.Position.xyz;
    float3 lightColor = light.Color.xyz * float3(light.Brightness, light.Brightness, light.Brightness);

    float3 L = normalize(lightPos - Input.WorldPos.xyz);
    float3 H = normalize(V + L);
    float distance = length(lightPos - Input.WorldPos.xyz);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = lightColor * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float3 numerator = F * G * NDF;
    float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo.xyz / PI + specular) * radiance * NdotL;
}

float3 CalcDirectionalLight(FragmentData Input, DirectionalLight light, float3 V, float3 N, float3 F0, float roughness, float metallic, float4 albedo)
{
    float3 lightColor = light.Color.xyz;

    float3 L = normalize(light.Direction.xyz);
    float3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float3 radiance = lightColor * NdotL;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float3 numerator = F * G * NDF;
    float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;

    return (kD * albedo.xyz / PI + specular) * radiance;
}

float ShadowCalculation(FragmentData Input)
{
    float3 projectionCoords = Input.LightPos.xyz / Input.LightPos.w;
    projectionCoords.xy = projectionCoords.xy * 0.5 + 0.5;
    projectionCoords.y = 1.0 - projectionCoords.y;

    float closestDepth = ShadowMap.SampleCmp(ShadowSampler, projectionCoords.xy, 0).r;
    float currentDepth = projectionCoords.z;

    float bias = max(0.05 * (1.0 - dot(Input.Normals, LightBuffer.Sun.Direction.xyz)), 0.005);
    
    float shadowWidth, shadowHeight;
    ShadowMap.GetDimensions(shadowWidth, shadowHeight);

    float shadow = 0.0;
    float2 texelSize = 1.0 / float2(shadowWidth, shadowHeight);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = ShadowMap.SampleCmpLevelZero(ShadowSampler, projectionCoords.xy + float2(x, y) * texelSize, currentDepth).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.005;
        }    
    }
    shadow /= 9.0;

    if (projectionCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

float4 GetPositionFromDepth(float2 uv, float depth, column_major float4x4 inverseViewProj)
{
    // Don't need to normalize -- directx depth is [0; 1]
	float z = depth;

    float4 clipSpacePosition = float4(uv * 2.0 - 1.0, z, 1.0);
    clipSpacePosition.y *= -1.0f;

    float4 viewSpacePosition = mul(inverseViewProj, clipSpacePosition);
    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition;
}

static const float MAX_REFLECTION_LOD = 4.0;

float4 Main(FragmentIn Input) : SV_Target
{
    FragmentData data;

    // Depth
    float depth = DepthBuffer.SampleCmp(ShadowSampler, Input.TexCoords, 0.0).r;
    float shadowDepth = ShadowMap.SampleCmp(ShadowSampler, Input.TexCoords, 0.0).r;

    // Position
    float4 position = GetPositionFromDepth(Input.TexCoords, depth, SceneBuffer.CameraProjViewInv);
    float4 shadowPosition = mul(SceneBuffer.SunView, position);

    // Normals
    float4 normals = Normals.Sample(Sampler, Input.TexCoords);

    data.Normals = normals.rgb;
    data.WorldPos = position;
    data.LightPos = shadowPosition;

    // Albedo + Emissive
    float4 albedo = AlbedoEmissive.Sample(Sampler, Input.TexCoords);
    albedo.xyz = pow(albedo.xyz, float3(2.2, 2.2, 2.2));

    // PBR + AO
    float4 PBRAO = PbrAO.Sample(Sampler, Input.TexCoords);
    float metallic = PBRAO.r;
    float roughness = PBRAO.g;
    float ao = PBRAO.b;

    // Shadow
    float shadow = ShadowCalculation(data);

    // Do light calcs!

    float3 N = normalize(normals.xyz);
    float3 V = normalize(SceneBuffer.CameraPosition - position).xyz;
    float3 R = reflect(-V, N);

    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo.xyz, metallic);

    float3 Lo = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < LightBuffer.PointLightCount; i++) {
        Lo += CalcPointLight(data, LightBuffer.PointLights[i], V, N, F0, roughness, metallic, albedo);
    }
    if (LightBuffer.HasSun) {
        Lo += CalcDirectionalLight(data, LightBuffer.Sun, V, N, F0, roughness, metallic, albedo) * (1.0 - shadow);
    }

    float3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    float3 ambient = float3(0.0, 0.0, 0.0);

    if (OutputData.IBL == true) {
        float3 irradiance = Irradiance.SampleLevel(Sampler, N, 0).rgb;
        float3 diffuse = irradiance * albedo.xyz;

        float3 prefilteredColor = Prefilter.SampleLevel(Sampler, R, roughness * MAX_REFLECTION_LOD).rgb;
        float2 brdvUV = float2(max(dot(N, V), 0.0), roughness);
        float2 brdf = BRDF.Sample(Sampler, brdvUV).rg;
        float3 specular = prefilteredColor * (F * brdf.x + brdf.y);
        ambient = (kD * diffuse + specular) * ao;
    } else {
        ambient = kD * albedo.xyz * ao;
    }

    ambient *= 0.5;

    float3 color = (ambient + Lo);
    float4 final = float4(0.0, 0.0, 0.0, 1.0);

    switch (OutputData.Mode) {
        case MODE_DEFAULT:
            final = float4(color, 1.0);
            break;
        case MODE_ALBEDO:
            final = albedo;
            break;
        case MODE_NORMAL:
            final = float4(normals.rgb, 1.0);
            break;
        case MODE_MR:
            final = PBRAO;
            break;
        case MODE_AO:
            final = float4(ao, ao, ao, 1.0);
            break;
        case MODE_EMISSIVE:
            final = albedo;
            break;
        case MODE_SPECULAR:
            final = float4(Lo, 1.0);
            break;
        case MODE_AMBIENT:
            final = float4(ambient, 1.0);
            break;
        case MODE_POSITION:
            final = position;
            break;
    }

    return final;
}
