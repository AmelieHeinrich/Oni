/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 00:39:47
 */

#include "shaders/Common/Math.hlsl"

struct Constants
{
	uint Equi;
	uint Cube;
	uint CubeSampler;
};

ConstantBuffer<Constants> Settings : register(b0);

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
	Texture2D Equi = ResourceDescriptorHeap[Settings.Equi];
	RWTexture2DArray<half4> Cube = ResourceDescriptorHeap[Settings.Cube];
	SamplerState CubeSampler = SamplerDescriptorHeap[Settings.CubeSampler];

	float outputWidth, outputHeight, outputDepth;
	Cube.GetDimensions(outputWidth, outputHeight, outputDepth);

    float2 st = ThreadID.xy / float2(outputWidth, outputHeight);
    float2 uv = 2.0 * float2(st.x, 1.0 - st.y) - float2(1.0, 1.0);

	// Select vector based on cubemap face index.
	float3 ret = float3(1.0f, 1.0f, 1.0f);
	switch(ThreadID.z)
	{
		case 0: ret = float3( 1.0,   uv.y, -uv.x); break;
		case 1: ret = float3(-1.0,   uv.y,  uv.x); break;
		case 2: ret = float3( uv.x,  1.0,  -uv.y); break;
		case 3: ret = float3( uv.x, -1.0,   uv.y); break;
		case 4: ret = float3( uv.x,  uv.y,  1.0); break;
		case 5: ret = float3(-uv.x,  uv.y, -1.0); break;
	}
    float3 v = normalize(ret);
	
	// Convert Cartesian direction vector to spherical coordinates.
	float phi   = atan2(v.z, v.x);
	float theta = acos(v.y);

	// Sample equirectangular texture.
	float4 color = Equi.SampleLevel(CubeSampler, float2(phi / TwoPI, theta / PI), 1.0);

	// Write out color to output cubemap.
	Cube[ThreadID] = color;
}