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
    float ModulationFactor;
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
    
    float3 currentColor = Current[ThreadID.xy].rgb;
    float3 previousColor = History.Sample(LinearSampler, uv).rgb;
    float3 output = currentColor * 0.1 + previousColor * 0.9;

    Current[ThreadID.xy] = float4(output, 1.0);
}
