/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:53:11
 */

#define MAX_LIGHTS 512
#define PI 3.14159265359

#define MODE_DEFAULT 0
#define MODE_ALBEDO 1
#define MODE_NORMAL 2
#define MODE_MR 3
#define MODE_AO 4
#define MODE_EMISSIVE 5
#define MODE_SPECULAR 6
#define MODE_AMBIENT 7

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

struct DirectionalLight
{
    float4 Position;
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

struct OutputBuffer
{
    int Mode;
    uint _Padding0;
    uint _Padding1;
    uint _Padding2;
};

Texture2D Texture : register(t2);
Texture2D NormalTexture : register(t3);
Texture2D AOTexture : register(t4);

SamplerState Sampler : register(s5);
ConstantBuffer<LightData> LightBuffer : register(b6);
ConstantBuffer<OutputBuffer> ModeBuffer : register(b7);

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

float3 CalcPointLight(FragmentIn Input, PointLight light, float3 N, float3 V)
{
    float3 lightDir = normalize(light.Position.xyz - Input.WorldPos.xyz);
    float3 halfDir = normalize(lightDir + V);
    float diff = max(dot(N, lightDir), 0.0);
    float3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(N, halfDir), 0.0), 1.0);
    float distance    = length(light.Position.xyz - Input.WorldPos.xyz);
    float attenuation = 1.0 / (distance * distance);
    float3 ambient  = light.Color.xyz * Texture.Sample(Sampler, Input.TexCoords).xyz;
    float3 diffuse  = light.Color.xyz * diff * Texture.Sample(Sampler, Input.TexCoords).xyz;
    float3 specular = light.Color.xyz * spec * Texture.Sample(Sampler, Input.TexCoords).xyz;
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

float3 CalcDirLight(FragmentIn Input, DirectionalLight light, float3 N, float3 V)
{
    float3 lightDir = normalize(-light.Direction.xyz);
    float diff = max(dot(N, lightDir), 0.0);
    float3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(V, reflectDir), 0.0), 1.0);
    float3 ambient  = light.Color.xyz * Texture.Sample(Sampler, Input.TexCoords).xyz;
    float3 diffuse  = light.Color.xyz * diff * Texture.Sample(Sampler, Input.TexCoords).xyz;
    float3 specular = light.Color.xyz * spec * Texture.Sample(Sampler, Input.TexCoords).xyz;
    return (ambient + diffuse + specular);
}  

float4 Main(FragmentIn Input) : SV_TARGET
{
    float4 albedo = Texture.Sample(Sampler, Input.TexCoords);
    if (albedo.a < 0.25)
        discard;
    float3 normal = GetNormalFromMap(Input);
    float4 aot = AOTexture.Sample(Sampler, Input.TexCoords);
    float ao = aot.r;

    float4 final = float4(0.0, 0.0, 0.0, 0.0);

    float3 N = normalize(normal);
    float3 V = normalize(Input.CameraPosition.xyz - Input.WorldPos.xyz);

    float3 diffuse = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < LightBuffer.PointLightCount; i++) {
        diffuse += CalcPointLight(Input, LightBuffer.PointLights[i], N, V);
    }
    if (LightBuffer.HasSun) {
        diffuse += CalcDirLight(Input, LightBuffer.Sun, N, V);
    }

    switch (ModeBuffer.Mode)
    {
        case MODE_DEFAULT:
            final = float4(diffuse, 1.0);
            break;
        case MODE_ALBEDO:
            final = albedo;
            break;
        case MODE_NORMAL:
            final = float4(normal, 1.0);
            break;
        case MODE_MR:
            final = float4(0.0, 0.0, 0.0, 0.0);
            break;
        case MODE_AO:
            final = aot;
            break;
        case MODE_EMISSIVE:
            final = float4(0.0, 0.0, 0.0, 0.0);
            break;
        case MODE_SPECULAR:
            final = LightBuffer.PointLights[0].Color;
            break;
        case MODE_AMBIENT:
            final = float4(0.0, 0.0, 0.0, 0.0);
            break;
    }

    return final;
}