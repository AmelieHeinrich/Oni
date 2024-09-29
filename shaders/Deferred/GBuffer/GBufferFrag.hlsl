//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-19 14:05:32
//

#define ORCA 1

struct FragmentIn
{
    float4 Position : SV_POSITION;
    float4 PrevPosition : POSITION0;
    float4 CurrPosition : POSITION1;
    float4 TexCoords : TEXCOORD;
    float4 Normals: NORMAL;
};

struct FragmentOut
{
    float4 Normals : SV_TARGET0;
    float4 AlbedoEmissive : SV_TARGET1;
    float4 PbrAO : SV_TARGET2;
    float4 Emissive: SV_TARGET3;
    float2 Velocity: SV_TARGET4;
};

struct SceneData
{
    // 36
    uint ModelBuffer;
    uint AlbedoTexture;
    uint NormalTexture; 
    uint PBRTexture;
    uint EmissiveTexture;
    uint AOTexture;
    uint Sampler;
    float2 Jitter;
    
    // 28 bytes
    float4 _Pad1;
    float3 _Pad2;
};

ConstantBuffer<SceneData> Settings : register(b0);

float3 GetNormalFromMap(FragmentIn Input)
{
    Texture2D NormalTexture = ResourceDescriptorHeap[Settings.NormalTexture];
    SamplerState Sampler = SamplerDescriptorHeap[Settings.Sampler];

    float3 hasNormalTexture = NormalTexture.Sample(Sampler, float2(0.0, 0.0)).rgb;
    if (hasNormalTexture.r == 1.0 && hasNormalTexture.g == 1.0 && hasNormalTexture.b == 1.0) {
        return normalize(Input.Normals.xyz);
    }

    float3 tangentNormal = NormalTexture.Sample(Sampler, Input.TexCoords.xy).rgb * 2.0 - 1.0;

    float3 Q1 = normalize(ddx(Input.Position.xyz));
    float3 Q2 = normalize(ddy(Input.Position.xyz));
    float2 ST1 = normalize(ddx(Input.TexCoords.xy));
    float2 ST2 = normalize(ddy(Input.TexCoords.xy));

    float3 N = normalize(Input.Normals.xyz);
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

    float4 albedo = AlbedoTexture.Sample(Sampler, Input.TexCoords.xy);
    if (albedo.a < 0.001)
        discard;

    float4 emission = EmissiveTexture.Sample(Sampler, Input.TexCoords.xy);
    float4 metallicRoughness = PBRTexture.Sample(Sampler, Input.TexCoords.xy);
    float4 aot = AOTexture.Sample(Sampler, Input.TexCoords.xy);

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
    output.AlbedoEmissive = float4(albedo.rgb, 1.0f);
    output.PbrAO = float4(float3(metallic, roughness, ao), 1.0f);
    output.Emissive = float4(emission.rgb, 1.0f) * 2.0f;
    output.Velocity = positionDifference;

    return output;
}
