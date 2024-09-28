//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-27 21:26:57
//

#include "shaders/Common/Compute.hlsl"

struct Constants
{
    uint History;
    uint Output;
    uint Velocity;
    uint LinearSampler;
    uint PointSampler;
};

ConstantBuffer<Constants> Settings : register(b0);

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    SamplerState PointSampler = SamplerDescriptorHeap[Settings.PointSampler];
    SamplerState LinearSampler = SamplerDescriptorHeap[Settings.LinearSampler];

    Texture2D<float2> VelocityTexture = ResourceDescriptorHeap[Settings.Velocity];
    Texture2D<float4> History = ResourceDescriptorHeap[Settings.History];
    RWTexture2D<float4> Current = ResourceDescriptorHeap[Settings.Output];

    // Start shader
    int width, height;
    VelocityTexture.GetDimensions(width, height);

    float2 texelSize = 1.0 / float2(width, height);
    float2 uv = TexelToUV(ThreadID.xy, texelSize);
    
    float2 velocityUV = VelocityTexture.Sample(PointSampler, uv);
    float2 reprojectedUV = uv + (-velocityUV);

    float3 currentColor = Current[ThreadID.xy].rgb;
    float3 previousColor = History.Sample(LinearSampler, reprojectedUV).rgb;
    
    float3 minColor = 9999.0;
    float3 maxColor = -9999.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float3 color = Current[ThreadID.xy + uint2(x, y)].rgb;
            minColor = min(minColor, color);
            maxColor = max(maxColor, color);
        }
    }
    float3 previousColorClamped = clamp(previousColor, minColor, maxColor);

    float3 output = currentColor * 0.1 + previousColorClamped * 0.9;
    Current[ThreadID.xy] = float4(output, 1.0);
}
