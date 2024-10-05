//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-19 15:01:31
//

#include "shaders/Common/Math.hlsl"
#include "shaders/Common/Lights.hlsl"
#include "shaders/Common/PBR.hlsl"
#include "shaders/Common/Compute.hlsl"

#define MODE_DEFAULT 0
#define MODE_ALBEDO 1
#define MODE_NORMAL 2
#define MODE_MR 3
#define MODE_BAKED_AO 4
#define MODE_SSAO 5
#define MODE_EMISSIVE 6
#define MODE_SPECULAR 7
#define MODE_AMBIENT 8
#define MODE_POSITION 9
#define MODE_VELOCITY 10

struct SceneData
{
    column_major float4x4 CameraMatrix;
    column_major float4x4 CameraProjViewInv;
    column_major float4x4 SunMatrix;
    float4 CameraPosition;
};

struct OutputBuffer
{
    int Mode;
    bool IBL;
    uint _Padding1;
    uint _Padding2;
};

struct FragmentData
{
    float3 Normals;
    float4 WorldPos;
    float4 LightPos;
};

struct Constants
{
    uint DepthBuffer;
    uint Normals;
    uint AlbedoEmissive;
    uint PbrAO;
    uint Velocity;
    uint Emissive;
    uint SSAO;

    uint Irradiance;
    uint Prefilter;
    uint BRDF;
    
    uint ShadowMap;
    uint Sampler;
    uint CubeSampler;
    uint ShadowSampler;
    
    uint SceneBuffer;
    uint LightBuffer;
    uint OutputData;
    uint HDRBuffer;
    
    float Direct;
    float Indirect;
};

ConstantBuffer<Constants> Settings : register(b0);

float3 CalcPointLight(FragmentData Input, PointLight light, float3 V, float3 N, float3 F0, float roughness, float metallic, float4 albedo)
{
    float3 lightPos = light.Position.xyz;
    float3 lightColor = light.Color.xyz * float3(light.Brightness, light.Brightness, light.Brightness);

    float3 L = normalize(lightPos - Input.WorldPos.xyz);
    float3 H = normalize(V + L);
    float distance = length(lightPos - Input.WorldPos.xyz) + Epsilon;
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = lightColor * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), Epsilon), F0);
    
    float3 numerator = F * G * NDF;
    float denominator = 4 * max(dot(N, V), Epsilon) * max(dot(N, L), Epsilon) + Epsilon;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), Epsilon);

    return (kD * albedo.xyz / PI + specular) * radiance * NdotL;
}

float3 CalcDirectionalLight(FragmentData Input, DirectionalLight light, float3 V, float3 N, float3 F0, float roughness, float metallic, float4 albedo)
{
    float3 lightColor = light.Color.xyz;

    float3 L = normalize(light.Direction.xyz);
    float3 H = normalize(V + L);
    float NdotL = max(dot(N, L), Epsilon);
    float3 radiance = lightColor * NdotL;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), Epsilon), F0);
    
    float3 numerator = F * G * NDF;
    float denominator = 4 * max(dot(N, V), Epsilon) * max(dot(N, L), Epsilon) + Epsilon;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = 1.0 - F;
    kD *= 1.0 - metallic;

    return (kD * albedo.xyz / PI + specular) * radiance;
}

float ShadowCalculation(FragmentData Input, Texture2D ShadowMap, SamplerComparisonState ShadowSampler, LightData LightBuffer)
{
    float3 projectionCoords = Input.LightPos.xyz / Input.LightPos.w;
    projectionCoords.xy = projectionCoords.xy * 0.5 + 0.5;
    projectionCoords.y = 1.0 - projectionCoords.y;

    float closestDepth = ShadowMap.SampleCmp(ShadowSampler, projectionCoords.xy, 0).r;
    float currentDepth = projectionCoords.z;

    float bias = max(0.05 * (1.0 - dot(Input.Normals, LightBuffer.Sun.Direction.xyz)), 0.005);
    
    float shadowWidth, shadowHeight;
    ShadowMap.GetDimensions(shadowWidth, shadowHeight);

    float shadow = Epsilon;
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

uint GetMaxReflectionLOD()
{
    TextureCube Prefilter = ResourceDescriptorHeap[Settings.Prefilter];

    int w, h, l;
    Prefilter.GetDimensions(0, w, h, l);
    return l;
}

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID) 
{
    // Reconstruct all textures
    Texture2D DepthBuffer = ResourceDescriptorHeap[Settings.DepthBuffer];
    Texture2D Normals = ResourceDescriptorHeap[Settings.Normals];
    Texture2D AlbedoEmissive = ResourceDescriptorHeap[Settings.AlbedoEmissive];
    Texture2D PbrAO = ResourceDescriptorHeap[Settings.PbrAO];
    Texture2D<float> SSAO = ResourceDescriptorHeap[Settings.SSAO];
    Texture2D<float2> Velocity = ResourceDescriptorHeap[Settings.Velocity];
    Texture2D<float4> Emissive = ResourceDescriptorHeap[Settings.Emissive];
    TextureCube Irradiance = ResourceDescriptorHeap[Settings.Irradiance];
    TextureCube Prefilter = ResourceDescriptorHeap[Settings.Prefilter];
    Texture2D BRDF = ResourceDescriptorHeap[Settings.BRDF];
    Texture2D ShadowMap = ResourceDescriptorHeap[Settings.ShadowMap];
    SamplerState Sampler = SamplerDescriptorHeap[Settings.Sampler];
    SamplerState CubeSampler = SamplerDescriptorHeap[Settings.CubeSampler];
    SamplerComparisonState ShadowSampler = SamplerDescriptorHeap[Settings.ShadowSampler];
    ConstantBuffer<SceneData> SceneBuffer = ResourceDescriptorHeap[Settings.SceneBuffer];
    ConstantBuffer<LightData> LightBuffer = ResourceDescriptorHeap[Settings.LightBuffer];
    ConstantBuffer<OutputBuffer> OutputData = ResourceDescriptorHeap[Settings.OutputData];
    RWTexture2D<float4> HDRBuffer = ResourceDescriptorHeap[Settings.HDRBuffer];

    // Start

    int width, height;
    HDRBuffer.GetDimensions(width, height);

    if (ThreadID.x > width || ThreadID.y > height) {
        return;
    }

    float2 TexCoords = TexelToUV(ThreadID.xy, 1.0 / float2(width, height));

    FragmentData data;

    // Depth
    float depth = DepthBuffer.SampleCmp(ShadowSampler, TexCoords, 0.0).r;

    // Position
    float4 position = GetPositionFromDepth(TexCoords, depth, SceneBuffer.CameraProjViewInv);
    float4 shadowPosition = mul(SceneBuffer.SunMatrix, position);

    // Normals
    float4 normals = Normals.Sample(Sampler, TexCoords);

    data.Normals = normals.rgb;
    data.WorldPos = position;
    data.LightPos = shadowPosition;

    // PBR + AO
    float4 PBRAO = PbrAO.Sample(Sampler, TexCoords);
    float metallic = PBRAO.r;
    float roughness = PBRAO.g;
    float ao = PBRAO.b;
    float ssao = SSAO.Sample(Sampler, TexCoords);

    // Albedo + Emissive
    float4 albedo = AlbedoEmissive.Sample(Sampler, TexCoords) + (Emissive.Sample(Sampler, TexCoords) * 5.0f);
    albedo.xyz = pow(albedo.xyz, float3(2.2, 2.2, 2.2));

    // Shadow
    float shadow = ShadowCalculation(data, ShadowMap, ShadowSampler, LightBuffer);

    // Velocity
    float2 velocity = Velocity.Sample(Sampler, TexCoords);

    // Do light calcs!

    float3 N = normalize(normals.xyz);
    float3 V = normalize(SceneBuffer.CameraPosition - position).xyz;
    float3 R = reflect(-V, N);

    float3 F0 = 0.04;
    F0 = lerp(F0, albedo.xyz, metallic);

    // .5 so to make sure there is no black spots
    float NdotV = max(0.5, dot(N, V));
    float3 Lr = 2.0 * NdotV * N - V;
    float3 Lo = 0.0;

    float3 directLighting = Epsilon;
    {
        for (int i = 0; i < LightBuffer.PointLightCount; i++) {
            Lo += CalcPointLight(data, LightBuffer.PointLights[i], V, N, F0, roughness, metallic, albedo);
        }
        if (LightBuffer.HasSun) {
            Lo += CalcDirectionalLight(data, LightBuffer.Sun, V, N, F0, roughness, metallic, albedo) * (1.0 - shadow);
        }
        directLighting += Lo;
    }

    float3 indirectLighting = Epsilon;
    {
        float3 irradiance = Irradiance.Sample(CubeSampler, N).rgb;
        float3 F = FresnelSchlick(NdotV, F0);
        float3 kd = lerp(1.0 - F, 0.0, metallic);
        float3 diffuseIBL = kd * albedo.rgb * irradiance;

        uint maxReflectionLOD = GetMaxReflectionLOD();
        float3 specularIrradiance = Prefilter.SampleLevel(CubeSampler, Lr, roughness * maxReflectionLOD).rgb;
        float2 specularBRDF = BRDF.Sample(Sampler, float2(NdotV, roughness)).rg;
        float3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
        indirectLighting = diffuseIBL + specularIBL;
    }

    directLighting *= ssao;

    directLighting *= Settings.Direct;
    indirectLighting *= Settings.Indirect;

    float3 color = directLighting + indirectLighting;
    float4 final = float4(0.0, 0.0, 0.0, 1.0);

    switch (OutputData.Mode) {
        case MODE_DEFAULT:
            final = float4(color, 1.0);
            break;
        case MODE_ALBEDO:
            final = albedo;
            break;
        case MODE_NORMAL:
            final = float4(normals.rgb * 0.5 + 0.5, 1.0);
            break;
        case MODE_MR:
            final = roughness;
            break;
        case MODE_BAKED_AO:
            final = float4(ao, ao, ao, 1.0);
            break;
        case MODE_SSAO:
            final = float4(ssao, ssao, ssao, 1.0);
            break;
        case MODE_EMISSIVE:
            final = Emissive.Sample(Sampler, TexCoords);
            break;
        case MODE_SPECULAR:
            final = float4(directLighting, 1.0);
            break;
        case MODE_AMBIENT:
            final = float4(indirectLighting, 1.0);
            break;
        case MODE_POSITION:
            final = position;
            break;
        case MODE_VELOCITY:
            final = float4(velocity, 0.0, 1.0);
            break;
    }

    HDRBuffer[ThreadID.xy] = final;
}
