//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-19 14:05:32
//

struct FragmentIn
{
    float4 Position : SV_POSITION;
    float2 TexCoords : TEXCOORD;
    float3 Normals: NORMAL;
};

struct FragmentOut
{
    float4 Normals : SV_TARGET0;
    float4 AlbedoEmissive : SV_TARGET1;
    float4 PbrAO : SV_TARGET2;
};

struct SceneData
{
    column_major float4x4 CameraMatrix;
    column_major float4x4 Transform;

    uint AlbedoTexture;
    uint NormalTexture; 
    uint PBRTexture;
    uint EmissiveTexture;
    uint AOTexture;
    uint Sampler;
    uint2 _Pad0;
};

ConstantBuffer<SceneData> Settings : register(b0);

float3 GetNormalFromMap(FragmentIn Input)
{
    Texture2D NormalTexture = ResourceDescriptorHeap[Settings.NormalTexture];
    SamplerState Sampler = SamplerDescriptorHeap[Settings.Sampler];

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

FragmentOut Main(FragmentIn Input)
{
    Texture2D AlbedoTexture = ResourceDescriptorHeap[Settings.AlbedoTexture];
    Texture2D NormalTexture = ResourceDescriptorHeap[Settings.NormalTexture];
    Texture2D PBRTexture = ResourceDescriptorHeap[Settings.PBRTexture];
    Texture2D EmissiveTexture = ResourceDescriptorHeap[Settings.EmissiveTexture];
    Texture2D AOTexture = ResourceDescriptorHeap[Settings.AOTexture];
    SamplerState Sampler = SamplerDescriptorHeap[Settings.Sampler];

    float4 albedo = AlbedoTexture.Sample(Sampler, Input.TexCoords);
    float4 emission = EmissiveTexture.Sample(Sampler, Input.TexCoords);
    float4 metallicRoughness = PBRTexture.Sample(Sampler, Input.TexCoords);
    float metallic = metallicRoughness.b;
    float roughness = metallicRoughness.g;
    float4 aot = AOTexture.Sample(Sampler, Input.TexCoords);
    float ao = aot.r;

    FragmentOut output = (FragmentOut)0;
    
    output.Normals = float4(GetNormalFromMap(Input), 1.0f);
    output.AlbedoEmissive = float4(albedo.rgb + emission.rgb, 1.0f);
    output.PbrAO = float4(float3(metallic, roughness, ao), 1.0f);

    return output;
}
