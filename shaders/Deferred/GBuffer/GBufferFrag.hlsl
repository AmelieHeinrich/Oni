//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-19 14:05:32
//

#include "shaders/Common/Math.hlsl"

#define ORCA 0

struct FragmentIn
{
    float4 Position : SV_POSITION;
    float4 PrevPosition : POSITION0;
    float4 CurrPosition : POSITION1;
    float3 Normals: NORMAL;
    uint MeshletIndex : COLOR0;
    float2 TexCoords : TEXCOORD;
    float2 Pad : POSITION2;
};

struct FragmentOut
{
    float4 Normals : SV_TARGET0;
    float4 AlbedoEmissive : SV_TARGET1;
    float4 PbrAO : SV_TARGET2;
    float4 Emissive: SV_TARGET3;
    float2 Velocity: SV_TARGET4;
};

struct PushConstants
{
    uint Matrices;
    uint VertexBuffer;
    uint IndexBuffer;
    uint MeshletBuffer;
    uint MeshletVertices;
    uint MeshletTriangleBuffer;
    uint MeshletBoundsBuffer;

    uint AlbedoTexture;
    uint NormalTexture;
    uint PBRTexture;
    uint EmissiveTexture;
    uint AOTexture;
    uint Sampler;
    
    uint DrawMeshlets;
    float EmissiveStrength;
    float2 Jitter;
    uint Pad;
};

ConstantBuffer<PushConstants> Settings : register(b0);

float3 GetNormalFromMap(FragmentIn Input)
{
    Texture2D NormalTexture = ResourceDescriptorHeap[Settings.NormalTexture];
    SamplerState Sampler = SamplerDescriptorHeap[Settings.Sampler];

    float3 hasNormalTexture = NormalTexture.Sample(Sampler, float2(0.0, 0.0)).rgb;
    if (hasNormalTexture.r == 1.0 && hasNormalTexture.g == 1.0 && hasNormalTexture.b == 1.0) {
        return normalize(Input.Normals.xyz);
    }

    float3 tangentNormal = NormalTexture.Sample(Sampler, Input.TexCoords.xy).rgb * 2.0 - 1.0;
    float3 normal = normalize(Input.Normals.xyz);

    float3 Q1 = ddx(Input.Position.xyz);
    float3 Q2 = ddy(Input.Position.xyz);
    float2 ST1 = ddx(Input.TexCoords.xy);
    float2 ST2 = ddy(Input.TexCoords.xy);

    float3 T = Q1 * ST2.y - Q2 * ST1.y + Epsilon;
    float3 B = cross(normal, T) + Epsilon;
    float3 N = normal + Epsilon;
    float3x3 TBN = float3x3(normalize(T), normalize(B), N);

    return normalize(mul(tangentNormal, TBN));
}

uint hash(uint a)
{
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return a;
}

FragmentOut Main(FragmentIn Input)
{
    Texture2D AlbedoTexture = ResourceDescriptorHeap[Settings.AlbedoTexture];
    Texture2D NormalTexture = ResourceDescriptorHeap[Settings.NormalTexture];
    Texture2D PBRTexture = ResourceDescriptorHeap[Settings.PBRTexture];
    Texture2D EmissiveTexture = ResourceDescriptorHeap[Settings.EmissiveTexture];
    Texture2D AOTexture = ResourceDescriptorHeap[Settings.AOTexture];
    SamplerState Sampler = SamplerDescriptorHeap[Settings.Sampler];

    float4 albedo = AlbedoTexture.Sample(Sampler, Input.TexCoords.xy);
    if (albedo.a < 0.1)
        discard;

    float4 emission = EmissiveTexture.Sample(Sampler, Input.TexCoords.xy);
    float4 metallicRoughness = PBRTexture.SampleLevel(Sampler, Input.TexCoords.xy, 0);
    float4 aot = AOTexture.SampleLevel(Sampler, Input.TexCoords.xy, 0);

    float ao = 1.0f;
    float roughness = 0.0f;
    float metallic = 0.0f;
    
    if (ORCA) {
        ao = metallicRoughness.r;
        roughness = metallicRoughness.g;
        metallic = metallicRoughness.b;
    } else {
        metallic = metallicRoughness.b;
        roughness = metallicRoughness.g;
        ao = aot.r;
    }

    FragmentOut output = (FragmentOut)0;
    
    float2 oldPos = Input.PrevPosition.xy / Input.PrevPosition.w;
    float2 newPos = Input.CurrPosition.xy / Input.CurrPosition.w;
    float2 positionDifference = (oldPos - newPos);
    positionDifference *= float2(-1.0f, 1.0f);

    output.Normals = float4(GetNormalFromMap(Input), 1.0f);
    if (Settings.DrawMeshlets == 1) {
        uint meshletHash = hash(Input.MeshletIndex);
        float3 meshletColor = float3(float(meshletHash & 255), float((meshletHash >> 8) & 255), float((meshletHash >> 16) & 255)) / 255.0;
        output.AlbedoEmissive = float4(meshletColor, 1.0f);
    } else {
        output.AlbedoEmissive = float4(albedo.rgb, 1.0f);
    }
    output.PbrAO = float4(float3(metallic, roughness, ao), 1.0f);
    output.Emissive = float4(emission.rgb, 1.0f) * Settings.EmissiveStrength;
    output.Velocity = positionDifference;

    return output;
}
