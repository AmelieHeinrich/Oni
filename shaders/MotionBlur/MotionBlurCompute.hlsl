//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-28 17:40:24
//

#include "shaders/Common/Compute.hlsl"

#define MAX_SAMPLES 16

struct Constants
{
    uint Velocity;
    uint Output;
    uint PointSampler;
    uint Samples;
};

ConstantBuffer<Constants> Settings : register(b0);

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    Texture2D<float2> VelocityTexture = ResourceDescriptorHeap[Settings.Velocity];
    RWTexture2D<float4> Output = ResourceDescriptorHeap[Settings.Output];
    SamplerState Sampler = SamplerDescriptorHeap[Settings.PointSampler];

    int width, height;
    VelocityTexture.GetDimensions(width, height);

    float2 texelSize = 1.0 / float2(width, height);
    float2 uv = TexelToUV(ThreadID.xy, texelSize);
    
    float2 velocity = VelocityTexture.Sample(Sampler, uv);

    float speed = length(velocity / texelSize);
    int nSamples = clamp(int(speed), 1, MAX_SAMPLES);
    
    float4 result = Output[ThreadID.xy];
    for (int i = 1; i < nSamples; i++) {
        float2 offset = velocity * (float(i) / float(nSamples - 1) - 0.5);
        uint2 newTexel = UVToTexel(uv + offset, uint2(width, height));
        result += Output[newTexel];
    }
    result /= float(nSamples);

    Output[ThreadID.xy] = result;
}
