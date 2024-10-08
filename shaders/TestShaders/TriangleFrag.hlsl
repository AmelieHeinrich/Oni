//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-08 17:40:27
//

struct MeshInput
{
    float4 Position : SV_POSITION;
    float3 Color    : COLOR;
};

struct Placeholder
{
    uint Data;
};

ConstantBuffer<Placeholder> Placeholder : register(b0);

float4 Main(MeshInput input) : SV_Target
{
    return float4(input.Color, 1.0);
}
