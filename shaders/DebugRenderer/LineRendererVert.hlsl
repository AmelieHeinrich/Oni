//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-15 10:29:29
//

struct VertexIn
{
    float4 Position: POSITION;
    float4 Col: COLOR;
};

struct VertexOut
{
    float4 Position: SV_POSITION;
    float4 Col: COLOR;
};

cbuffer Transform : register(b0)
{
	column_major float4x4 ModelViewProjection;
};

VertexOut Main(VertexIn Input)
{
    VertexOut output = (VertexOut)0;

    output.Position = mul(ModelViewProjection, float4(Input.Position.xyz, 1.0));
    output.Col = Input.Col;

    return output;
}
