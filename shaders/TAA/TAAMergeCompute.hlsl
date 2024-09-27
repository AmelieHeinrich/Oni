//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-27 22:05:38
//

#include "shaders/Common/Compute.hlsl"

struct Constants
{
    uint History;
    uint Current;
    uint PointSampler;
};

ConstantBuffer<Constants> Settings : register(b0);

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> History = ResourceDescriptorHeap[Settings.History];
    RWTexture2D<float4> Output = ResourceDescriptorHeap[Settings.Current];
    SamplerState PointSampler = SamplerDescriptorHeap[Settings.PointSampler];

    int width, height;
    History.GetDimensions(width, height);

    float2 texelSize = 1.0 / float2(width, height);
    float2 texcoords = TexelToUV(ThreadID.xy, texelSize);

    float3 historyColor = History.Sample(PointSampler, texcoords).rgb;
    float3 currentColor = Output[ThreadID.xy].rgb;
    float3 color = lerp(currentColor, historyColor, 0.9);
    
    Output[ThreadID.xy] = float4(color, 1.0f);
}
