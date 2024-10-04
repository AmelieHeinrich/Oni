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

float4 Upsample(uint3 ThreadID)
{
	Texture2D MipN = ResourceDescriptorHeap[Settings.MipN];
	SamplerState LinearSampler = SamplerDescriptorHeap[Settings.LinearSampler];
	RWTexture2D<float4> MipNMinusOne = ResourceDescriptorHeap[Settings.MipNMinusOne];

    int width, height;
    MipNMinusOne.GetDimensions(width, height);

    float2 uv = TexelToUV(ThreadID.xy, 1.0 / float2(width, height));
	float3 currentColor = MipN.Sample(LinearSampler, uv).xyz;

	//
	// a - b - c
	// d - e - f
	// g - h - i
	//
	float3 a = MipN.Sample(LinearSampler, uv, int2( -1.0f,  1.0f)).xyz;
	float3 b = MipN.Sample(LinearSampler, uv, int2(  0.0f,  1.0f)).xyz;
	float3 c = MipN.Sample(LinearSampler, uv, int2(  1.0f,  1.0f)).xyz;
	
	float3 d = MipN.Sample(LinearSampler, uv, int2( -1.0f,  0.0f)).xyz;
	float3 e = MipN.Sample(LinearSampler, uv, int2(  0.0f,  0.0f)).xyz;
	float3 f = MipN.Sample(LinearSampler, uv, int2(  1.0f,  0.0f)).xyz;
	
	float3 g = MipN.Sample(LinearSampler, uv, int2( -1.0f, -1.0f)).xyz;
	float3 h = MipN.Sample(LinearSampler, uv, int2(  0.0f, -1.0f)).xyz;
	float3 i = MipN.Sample(LinearSampler, uv, int2(  1.0f, -1.0f)).xyz;
    
	// Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
	float3 outColor = e * 4.0;
	outColor += (b + d + f + h) * 2.0;
	outColor += (a + c + g + i);
	outColor *= 1.0 / 16.0;

	return float4(lerp(currentColor, outColor, 1.0), 1.0);
}

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
	Texture2D MipN = ResourceDescriptorHeap[Settings.MipN];
	SamplerState LinearSampler = SamplerDescriptorHeap[Settings.LinearSampler];
	RWTexture2D<float4> MipNMinusOne = ResourceDescriptorHeap[Settings.MipNMinusOne];

    MipNMinusOne[ThreadID.xy] = Upsample(ThreadID);
}
