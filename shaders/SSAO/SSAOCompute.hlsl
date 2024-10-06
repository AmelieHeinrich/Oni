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
    float4 result = float4(uv * 2.0 - 1.0, depth, 1);
    result = mul(inverseProj, result);
    result.xyz /= result.w;
    return result;
}

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    Texture2D<float> Depth = ResourceDescriptorHeap[Settings.Depth];
    Texture2D<float3> Normal = ResourceDescriptorHeap[Settings.Normal];
    Texture2D<float4> Noise = ResourceDescriptorHeap[Settings.NoiseTexture];
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
    Noise.GetDimensions(noiseWidth, noiseHeight);

    float2 TexCoords = TexelToUV(ThreadID.xy, 1.0 / float2(width, height));
    float2 NoiseTexCoords = TexelToUV(ThreadID.xy, 1.0 / float2(noiseWidth, noiseHeight));

    // Get input data for SSAO
    float depth = Depth.Sample(PointClamp, TexCoords).r;
    float3 position = GetViewFromDepth(TexCoords, depth, Camera.ProjectionInv).xyz;
    float3 normal = normalize(Normal.Sample(PointClamp, TexCoords).xyz);
    float3 randomVec = Noise.Sample(PointWrap, NoiseTexCoords).xyz;

    // Create TBN
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);

    // Iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for (int i = 0; i < Settings.KernelSize; i++) {
        // Get sample position
        float3 samplePos = mul(Kernels.Samples[i], TBN);
        samplePos = samplePos * Settings.Radius + position;

        float3 sampleDir = normalize(samplePos - position);
        float nDotS = max(dot(normal, sampleDir), 0);

        // Create new sample point
        float4 offset = mul(float4(samplePos, 1.0), Camera.Projection);
        offset.xy /= offset.w;

        // Get sample depth
        float sampleDepth = Depth.Sample(PointClamp, float2(offset.x * 0.5 + 0.5, -offset.y * 0.5 + 0.5)).r;
        sampleDepth = GetViewFromDepth(offset.xy, sampleDepth, Camera.ProjectionInv).z;

        // Range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, Settings.Radius / abs(position.z - sampleDepth));
        occlusion += rangeCheck * step(sampleDepth, samplePos.z) * nDotS;
    }
    occlusion = 1.0 - (occlusion / Settings.KernelSize);
    occlusion = pow(occlusion, Settings.Power);

    Output[ThreadID.xy] = occlusion;
}
