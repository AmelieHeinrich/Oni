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

struct Constants
{
	uint EnvironmentMap;
	uint CubeSampler;
	uint2 _Pad0;
	column_major float4x4 ModelViewProjection;
};

ConstantBuffer<Constants> Settings : register(b0);

VertexOut Main(VertexIn input)
{
	VertexOut output = (VertexOut)0;
	output.Position = mul(Settings.ModelViewProjection, float4(input.Position, 1.0));
	output.LocalPosition = input.Position;
	return output;
}
