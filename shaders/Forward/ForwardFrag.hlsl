/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:53:11
 */

#define MAX_LIGHTS 512
#define PI 3.14159265359

struct FragmentIn
{
    float4 Position : SV_POSITION;
    float2 TexCoords : TEXCOORD;
    float3 Normals: NORMAL;
    float4 WorldPos : COLOR0;
    float4 CameraPosition: COLOR1;
    float4 FlatColor: COLOR2;
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
Texture2D PBRTexture : register(t4);
Texture2D EmissiveTexture : register(t5);
Texture2D AOTexture : register(t6);

TextureCube Irradiance : register(t7);
TextureCube Prefilter : register(t8);
Texture2D BRDF : register(t9);

SamplerState Sampler : register(s10);
ConstantBuffer<LightData> LightBuffer : register(b11);

float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
	return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 GetNormalFromMap(FragmentIn Input)
{
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

static const float MAX_REFLECTION_LOD = 4.0;

float4 Main(FragmentIn Input) : SV_TARGET
{
    float4 albedo = Texture.Sample(Sampler, Input.TexCoords) * Input.FlatColor;
    if (albedo.a < 0.25)
        discard;
    float4 emission = EmissiveTexture.Sample(Sampler, Input.TexCoords);
    float4 metallicRoughness = PBRTexture.Sample(Sampler, Input.TexCoords);
    float metallic = metallicRoughness.b;
    float roughness = metallicRoughness.g;
    float ao = AOTexture.Sample(Sampler, Input.TexCoords).r;

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
        float3 lightPos = LightBuffer.PointLights[i].Position.xyz;
        float3 lightColor = LightBuffer.PointLights[i].Color.xyz;

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

        Lo += (kD * albedo.xyz / PI + specular) * radiance * NdotL;
    }

    float3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    float3 irradiance = Irradiance.Sample(Sampler, N).rgb;
    float3 diffuse = irradiance * albedo.xyz;

    float3 prefilteredColor = Prefilter.SampleLevel(Sampler, R, roughness * MAX_REFLECTION_LOD).rgb;
    float2 brdf = BRDF.Sample(Sampler, float2(max(dot(N, V), 0.0), roughness)).rg;
    float3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    float3 ambient = (kD * diffuse + specular) * ao;
    float3 color = ambient + Lo;
    float4 final = float4(color, 1.0);

    return final;
}