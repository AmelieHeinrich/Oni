//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 01:38:58
//

//
//  https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/
//

#include "shaders/Common/Compute.hlsl"
#include "shaders/Common/Colors.hlsl"

struct Parameters
{
    float FilterRadius;
    uint MipN;
	uint LinearSampler;
	uint MipNMinusOne;
};

ConstantBuffer<Parameters> Settings : register(b0);

float3 Upsample(uint3 ThreadID)
{
	Texture2D MipN = ResourceDescriptorHeap[Settings.MipN];
	SamplerState LinearSampler = SamplerDescriptorHeap[Settings.LinearSampler];
	RWTexture2D<float3> MipNMinusOne = ResourceDescriptorHeap[Settings.MipNMinusOne];

    int width, height;
    MipNMinusOne.GetDimensions(width, height);

    float2 uv = TexelToUV(ThreadID.xy, 1.0 / float2(width, height));
	float3 currentColor = MipN.Sample(LinearSampler, uv).xyz;

	float3 outColor = 0;
	outColor += 0.0625f * MipN.Sample(LinearSampler, uv, int2( -1.0f,  1.0f)).xyz;
	outColor += 0.125f  * MipN.Sample(LinearSampler, uv, int2(  0.0f,  1.0f)).xyz;
	outColor += 0.0625f * MipN.Sample(LinearSampler, uv, int2(  1.0f,  1.0f)).xyz;
	outColor += 0.125f  * MipN.Sample(LinearSampler, uv, int2( -1.0f,  0.0f)).xyz;
	outColor += 0.25f   * MipN.Sample(LinearSampler, uv, int2(  0.0f,  0.0f)).xyz;
	outColor += 0.125f  * MipN.Sample(LinearSampler, uv, int2(  1.0f,  0.0f)).xyz;
	outColor += 0.0625f * MipN.Sample(LinearSampler, uv, int2( -1.0f, -1.0f)).xyz;
	outColor += 0.125f  * MipN.Sample(LinearSampler, uv, int2(  0.0f, -1.0f)).xyz;
	outColor += 0.0625f * MipN.Sample(LinearSampler, uv, int2(  1.0f, -1.0f)).xyz;
    return lerp(currentColor, outColor, Settings.FilterRadius);
}

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
	Texture2D MipN = ResourceDescriptorHeap[Settings.MipN];
	SamplerState LinearSampler = SamplerDescriptorHeap[Settings.LinearSampler];
	RWTexture2D<float3> MipNMinusOne = ResourceDescriptorHeap[Settings.MipNMinusOne];

    MipNMinusOne[ThreadID.xy] = Upsample(ThreadID);
}
