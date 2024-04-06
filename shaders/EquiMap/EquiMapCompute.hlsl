/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 00:39:47
 */

static const float PI = 3.141592;
static const float TwoPI = 2 * PI;

Texture2D Equi : register(t0);
RWTexture2DArray<half4> Cube : register(u1);
SamplerState CubeSampler : register(s2);

float3 GetSamplingVector(uint3 ThreadID)
{
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
    return normalize(ret);
}

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
	float3 v = GetSamplingVector(ThreadID);
	
	// Convert Cartesian direction vector to spherical coordinates.
	float phi   = atan2(v.z, v.x);
	float theta = acos(v.y);

	// Sample equirectangular texture.
	float4 color = Equi.SampleLevel(CubeSampler, float2(phi / TwoPI, theta / PI), 1.0);

	// Write out color to output cubemap.
	Cube[ThreadID] = color;
}