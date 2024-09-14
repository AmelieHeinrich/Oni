/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 00:40:29
 */

struct VertexIn
{
    float3 Position: POSTIION;
};

struct VertexOut
{
	float4 Position : SV_POSITION;
	float3 LocalPosition : COLOR0;
};

cbuffer Transform : register(b0)
{
	column_major float4x4 ModelViewProjection;
};

VertexOut Main(VertexIn input)
{
	VertexOut output = (VertexOut)0;
	output.Position = mul(ModelViewProjection, float4(input.Position, 1.0));
	output.LocalPosition = input.Position;
	return output;
}
