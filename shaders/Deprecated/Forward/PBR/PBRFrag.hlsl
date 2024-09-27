/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:53:11
 */

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

struct FragmentIn
{
    float4 Position : SV_POSITION;
    float2 TexCoords : TEXCOORD;
    float3 Normals: NORMAL;
    float4 WorldPos : COLOR0;
    float4 CameraPosition: COLOR1;
    float4 FragPosLightSpace: COLOR2;
};

struct OutputBuffer
{
    int Mode;
    bool IBL;
    uint _Padding1;
    uint _Padding2;
};

Texture2D Texture : register(t2);
Texture2D NormalTexture : register(t3);
Texture2D PBRTexture : register(t4);
Texture2D EmissiveTexture : register(t5);
Texture2D AOTexture : register(t6);

TextureCube Irradiance : register(t7);
TextureCube Prefilter : register(t8);
Texture2D BRDF : register(t9);
Texture2D ShadowMap : register(t10);

SamplerState Sampler : register(s11);
SamplerComparisonState ShadowSampler : register(s12);
ConstantBuffer<LightData> LightBuffer : register(b13);
ConstantBuffer<OutputBuffer> OutputData : register(b14);

float3 GetNormalFromMap(FragmentIn Input)
{
    float3 hasNormalTexture = NormalTexture.Sample(Sampler, float2(0.0, 0.0)).rgb;
    if (hasNormalTexture.r == 1.0 && hasNormalTexture.g == 1.0 && hasNormalTexture.b == 1.0) {
        return normalize(Input.Normals);
    }

    float3 tangentNormal = NormalTexture.Sample(Sampler, Input.TexCoords).rgb * 2.0 - 1.0;

    float3 Q1 = normalize(ddx(Input.Position.xyz));
    float3 Q2 = normalize(ddy(Input.Position.xyz));
    float2 ST1 = normalize(ddx(Input.TexCoords.xy));
    float2 ST2 = normalize(ddy(Input.TexCoords.xy));

    float3 N = normalize(Input.Normals);
    float3 T = normalize(Q1 * ST2.y - Q2 * ST1.y);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(tangentNormal, TBN));
}

float3 CalcPointLight(FragmentIn Input, PointLight light, float3 V, float3 N, float3 F0, float roughness, float metallic, float4 albedo)
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

float3 CalcDirectionalLight(FragmentIn Input, DirectionalLight light, float3 V, float3 N, float3 F0, float roughness, float metallic, float4 albedo)
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

float ShadowCalculation(FragmentIn Input)
{
    float3 projectionCoords = Input.FragPosLightSpace.xyz / Input.FragPosLightSpace.w;
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

static const float MAX_REFLECTION_LOD = 4.0;

float4 Main(FragmentIn Input) : SV_TARGET
{
    float4 albedo = Texture.Sample(Sampler, Input.TexCoords);
    if (albedo.a < 0.25)
        discard;
    float4 emission = EmissiveTexture.Sample(Sampler, Input.TexCoords);
    float4 metallicRoughness = PBRTexture.Sample(Sampler, Input.TexCoords);
    float metallic = metallicRoughness.b;
    float roughness = metallicRoughness.g;
    float4 aot = AOTexture.Sample(Sampler, Input.TexCoords);
    float ao = aot.r;
    float shadow = ShadowCalculation(Input);

    if (emission.x != 1.0f && emission.y != 1.0f && emission.y != 1.0f) {
        albedo.xyz += emission.xyz;
    }
    albedo.xyz = pow(albedo.xyz, float3(2.2, 2.2, 2.2));
    
    float3 normal = GetNormalFromMap(Input);

    float3 N = normalize(normal);
    float3 V = normalize(Input.CameraPosition - Input.WorldPos).xyz;
    float3 R = reflect(-V, N);

    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo.xyz, metallic);

    float3 Lo = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < LightBuffer.PointLightCount; i++) {
        Lo += CalcPointLight(Input, LightBuffer.PointLights[i], V, N, F0, roughness, metallic, albedo);
    }
    if (LightBuffer.HasSun) {
        Lo += CalcDirectionalLight(Input, LightBuffer.Sun, V, N, F0, roughness, metallic, albedo) * (1.0 - shadow);
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
    float4 final = float4(0.0, 0.0, 0.0, 0.0);

    switch (OutputData.Mode) {
        case MODE_DEFAULT:
            final = float4(color, 1.0);
            break;
        case MODE_ALBEDO:
            final = albedo;
            break;
        case MODE_NORMAL:
            final = float4(normal, 1.0);
            break;
        case MODE_MR:
            final = metallicRoughness;
            break;
        case MODE_AO:
            final = aot;
            break;
        case MODE_EMISSIVE:
            final = emission;
            break;
        case MODE_SPECULAR:
            final = float4(Lo, 1.0);
            break;
        case MODE_AMBIENT:
            final = float4(ambient, 1.0);
            break;
        case MODE_POSITION:
            final = Input.WorldPos;
            break;
    }

    return final;
}