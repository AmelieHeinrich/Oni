//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 01:38:51
//

//
//  https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/
//

#include "shaders/Common/Compute.hlsl"
#include "shaders/Common/Colors.hlsl"

#define KARIS_AVERAGE 0

struct Constants
{
	uint InputHDR;
	uint InputSampler;
	uint Downsampled;
};

ConstantBuffer<Constants> Settings : register(b0);

float3 ComputePartialAverage(float3 v0, float3 v1, float3 v2, float3 v3)
{
#if KARIS_AVERAGE
	float w0 = 1.0f / (1.0f + Luminance(v0));
	float w1 = 1.0f / (1.0f + Luminance(v1));
	float w2 = 1.0f / (1.0f + Luminance(v2));
	float w3 = 1.0f / (1.0f + Luminance(v3));
	return (v0 * w0 + v1 * w1 + v2 * w2 + v3 * w3) / (w0 + w1 + w2 + w3);
#else
	return 0.25f * (v0 + v1 + v2 + v3);
#endif
}

float3 Downsample(uint3 ThreadID)
{
	Texture2D InputHDR = ResourceDescriptorHeap[Settings.InputHDR];
	SamplerState InputSampler = SamplerDescriptorHeap[Settings.InputSampler];
	RWTexture2D<float3> Downsampled = SamplerDescriptorHeap[Settings.Downsampled];

    int width, height;
    Downsampled.GetDimensions(width, height);

    float2 uv = TexelToUV(ThreadID.xy, 1.0 / float2(width, height));

	float3 outColor = 0;
	float3 M0 = InputHDR.SampleLevel(InputSampler, uv, 0, int2(-1.0f,  1.0f)).xyz;
	float3 M1 = InputHDR.SampleLevel(InputSampler, uv, 0, int2( 1.0f,  1.0f)).xyz;
	float3 M2 = InputHDR.SampleLevel(InputSampler, uv, 0, int2(-1.0f, -1.0f)).xyz;
	float3 M3 = InputHDR.SampleLevel(InputSampler, uv, 0, int2( 1.0f, -1.0f)).xyz;

	float3 TL = InputHDR.SampleLevel(InputSampler, uv, 0, int2(-2.0f, 2.0f)).xyz;
	float3 T  = InputHDR.SampleLevel(InputSampler, uv, 0, int2( 0.0f, 2.0f)).xyz;
	float3 TR = InputHDR.SampleLevel(InputSampler, uv, 0, int2( 2.0f, 2.0f)).xyz;
	float3 L  = InputHDR.SampleLevel(InputSampler, uv, 0, int2(-2.0f, 0.0f)).xyz;
	float3 C  = InputHDR.SampleLevel(InputSampler, uv, 0, int2( 0.0f, 0.0f)).xyz;
	float3 R  = InputHDR.SampleLevel(InputSampler, uv, 0, int2( 2.0f, 0.0f)).xyz;
	float3 BL = InputHDR.SampleLevel(InputSampler, uv, 0, int2(-2.0f,-2.0f)).xyz;
	float3 B  = InputHDR.SampleLevel(InputSampler, uv, 0, int2( 0.0f,-2.0f)).xyz;
	float3 BR = InputHDR.SampleLevel(InputSampler, uv, 0, int2( 2.0f,-2.0f)).xyz;

	outColor += ComputePartialAverage(M0, M1, M2, M3) * 0.5f;
	outColor += ComputePartialAverage(TL, T, C, L) * 0.125f;
	outColor += ComputePartialAverage(TR, T, C, R) * 0.125f;
	outColor += ComputePartialAverage(BL, B, C, L) * 0.125f;
	outColor += ComputePartialAverage(BR, B, C, R) * 0.125f;

    return outColor;
}

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
	Texture2D InputHDR = ResourceDescriptorHeap[Settings.InputHDR];
	SamplerState InputSampler = SamplerDescriptorHeap[Settings.InputSampler];
	RWTexture2D<float3> Downsampled = SamplerDescriptorHeap[Settings.Downsampled];

    Downsampled[ThreadID.xy] = Downsample(ThreadID);
}
