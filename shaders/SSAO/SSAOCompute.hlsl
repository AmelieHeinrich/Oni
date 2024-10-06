//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-05 11:39:55
//

#include "shaders/Common/Compute.hlsl"

// 256 * 3
struct KernelInfo
{
    float3 Samples[64];
};

struct CameraInfo
{
    column_major float4x4 Projection;
    column_major float4x4 ProjectionInv;
};

struct Constants
{
    uint Depth;
    uint Normal;
    uint NoiseTexture;
    uint KernelBuffer;

    uint CameraBuffer;
    uint KernelSize;
    float Radius;
    float Bias;

    uint PointSampler;
    uint PointClampSampler;
    uint Output;
    uint Power;
};

ConstantBuffer<Constants> Settings : register(b0);

float4 GetViewFromDepth(float2 uv, float depth, column_major float4x4 inverseProj)
{
    float4 result = float4(float3(uv * 2. - 1., depth), 1.);
    result = mul(inverseProj, result);
    result.xyz /= result.w;
    result.y *= -1.0;
    return result;
}

float2 hash2(inout float HASH2SEED) 
{
    HASH2SEED += 0.1f;
    float2 x= frac(sin(float2(HASH2SEED, HASH2SEED + 0.1f)) * float2(43758.5453123, 22578.1459123));
    HASH2SEED += 0.2f;
	return x;
}

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    Texture2D Depth = ResourceDescriptorHeap[Settings.Depth];
    Texture2D Normal = ResourceDescriptorHeap[Settings.Normal];
    ConstantBuffer<KernelInfo> Kernels = ResourceDescriptorHeap[Settings.KernelBuffer];
    ConstantBuffer<CameraInfo> Camera = ResourceDescriptorHeap[Settings.CameraBuffer];
    SamplerState PointWrap = SamplerDescriptorHeap[Settings.PointSampler];
    SamplerState PointClamp = SamplerDescriptorHeap[Settings.PointClampSampler];
    RWTexture2D<float> Output = ResourceDescriptorHeap[Settings.Output];

    // Start shader
    int width, height;
    Depth.GetDimensions(width, height);
    if (ThreadID.x > width || ThreadID.y > height) {
        return;
    }

    int noiseWidth, noiseHeight;

    float2 NoiseScale = float2(width / noiseWidth, height / noiseHeight);
    float2 TexCoords = TexelToUV(ThreadID.xy, 1.0 / float2(width, height));
    
	float Hash2Seed;
	Hash2Seed = (TexCoords.x * TexCoords.y) * height;
	
    // Get input data for SSAO 
	float depth = Depth.Sample(PointClamp, TexCoords).r;
    float3 position = GetViewFromDepth(TexCoords, depth, Camera.ProjectionInv).xyz;
    float3 normal = normalize(Normal.Sample(PointClamp, TexCoords).xyz);
    float3 randomVec = float3(hash2(Hash2Seed), hash2(Hash2Seed).x);

    // Create TBN
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);
	
    // Iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for (int i = 0; i < Settings.KernelSize; i++) {
		float3 rv = normalize(float3(hash2(Hash2Seed)*2.-1.,hash2(Hash2Seed).x)) * hash2(Hash2Seed).x;
		float3 rr = float3(hash2(Hash2Seed),hash2(Hash2Seed).x) * 2. - 1.;
	
        // Get sample position
        float3 samplePos = mul(TBN, rv);
        samplePos = rv + position;
		
        // Create new sample point
        float4 offset = mul(Camera.Projection, float4(samplePos, 1.0));
        offset.xy /= offset.w;

        // Get sample depth
        float sampleDepth = Depth.Sample(PointClamp, float2(offset.x * 0.5 + 0.5, (-offset.y * 0.5 + 0.5))).r;
        sampleDepth = GetViewFromDepth(offset.xy, sampleDepth, Camera.ProjectionInv).z;

        float rangeCheck = smoothstep(0.0, 1.0, Settings.Radius / abs(position.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + Settings.Bias ? 1.0 : 0.0) * rangeCheck;  
    }
    occlusion = 1.0 - (occlusion / float(Settings.KernelSize));

    Output[ThreadID.xy] = saturate(pow(occlusion, Settings.Power));
}
