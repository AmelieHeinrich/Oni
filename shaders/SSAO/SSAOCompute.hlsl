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
    column_major float4x4 CameraProjViewInv;
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

float3 ComputePositionViewFromZ(uint2 coords, float zbuffer)
{
    ConstantBuffer<CameraInfo> Camera = ResourceDescriptorHeap[Settings.CameraBuffer];
    Texture2D Depth = ResourceDescriptorHeap[Settings.Depth];

    int screenWidth, screenHeight;
    Depth.GetDimensions(screenWidth, screenHeight);

	float2 screenPixelOffset = float2(2.0f, -2.0f) / float2(screenWidth, screenHeight);
    float2 positionScreen = (float2(coords.xy) + 0.5f) * screenPixelOffset.xy + float2(-1.0f, 1.0f);
	float viewSpaceZ = Camera.Projection._43 / (zbuffer - Camera.Projection._33);
	
    float2 screenSpaceRay = float2(positionScreen.x / Camera.Projection._11, positionScreen.y / Camera.Projection._22);
    float3 positionView;
    positionView.z = viewSpaceZ;
    positionView.xy = screenSpaceRay.xy * positionView.z;
    
    return positionView;
}

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    Texture2D Depth = ResourceDescriptorHeap[Settings.Depth];
    Texture2D Normal = ResourceDescriptorHeap[Settings.Normal];
    Texture2D Noise = ResourceDescriptorHeap[Settings.NoiseTexture];
    ConstantBuffer<KernelInfo> Kernels = ResourceDescriptorHeap[Settings.KernelBuffer];
    ConstantBuffer<CameraInfo> Camera = ResourceDescriptorHeap[Settings.CameraBuffer];
    SamplerState PointWrap = SamplerDescriptorHeap[Settings.PointSampler];
    SamplerState PointClamp = SamplerDescriptorHeap[Settings.PointClampSampler];
    RWTexture2D<float> Output = ResourceDescriptorHeap[Settings.Output];

    // Start shader
    int width, height;
    Depth.GetDimensions(width, height);

    int noiseWidth, noiseHeight;
    Noise.GetDimensions(noiseWidth, noiseHeight);
    float2 NoiseScale = float2(width / noiseWidth, height / noiseHeight);

    if (ThreadID.x > width || ThreadID.y > height) {
        return;
    }
    float2 TexCoords = TexelToUV(ThreadID.xy, 1.0 / float2(width, height));
    
    // Get input data for SSAO
    float depth = Depth.Sample(PointClamp, TexCoords).r;
    float3 position = ComputePositionViewFromZ(TexCoords, depth).xyz;
    float3 normal = normalize(Normal.Sample(PointClamp, TexCoords).xyz);
    float3 randomVec = normalize(Noise.Sample(PointWrap, TexCoords * NoiseScale).xyz);

    // Create TBN
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);

    // Iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for (int i = 0; i < Settings.KernelSize; i++) {
        // Get sample position
        float3 samplePos = mul(Kernels.Samples[i], TBN);
        samplePos += samplePos + Settings.Radius * position;

        float4 offset = float4(samplePos, 1.0);
        offset = mul(offset, Camera.Projection);
        offset.xyz /= offset.w;

        // Get sample depth
        float sampleDepth = Depth.Sample(PointClamp, offset.xy).r;
        sampleDepth = ComputePositionViewFromZ(offset.xy, sampleDepth).z;

        // Range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, Settings.Radius / abs(position.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + Settings.Bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / Settings.KernelSize);
    occlusion = pow(occlusion, Settings.Power);

    Output[ThreadID.xy] = occlusion;
}
