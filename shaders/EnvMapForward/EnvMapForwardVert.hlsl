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
	row_major float4x4 ModelViewProjection;
};

VertexOut Main(VertexIn input)
{
	VertexOut output = (VertexOut)0;
	output.Position = mul(float4(input.Position, 1.0), ModelViewProjection);
	output.LocalPosition = input.Position;
	return output;
}
