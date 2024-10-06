//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-06 16:39:22
//

struct VertexIn
{
    float4 Position: POSITION;
};

struct VertexOut
{
    float4 Position: SV_POSITION;
};

cbuffer Transform : register(b0)
{
	column_major float4x4 ViewProjection;
};

VertexOut Main(VertexIn Input)
{
    VertexOut output = (VertexOut)0;

    output.Position = mul(ViewProjection, Input.Position);

    return output;
}
