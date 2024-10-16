//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 15:24:51
//

#include "shaders/Common/Compute.hlsl"
#include "shaders/Common/Math.hlsl"

struct Parameters
{
    uint Input;
    uint InputSampler;
    uint OutputHDR;
    float BloomStrength;
};

ConstantBuffer<Parameters> Settings : register(b0);

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    Texture2D<float3> Input = ResourceDescriptorHeap[Settings.Input];
    SamplerState InputSampler = SamplerDescriptorHeap[Settings.InputSampler];
    RWTexture2D<float4> OutputHDR = ResourceDescriptorHeap[Settings.OutputHDR];

    uint width, height;
    Input.GetDimensions(width, height);

    float2 UVs = TexelToUV(ThreadID.xy, 1.0 / float2(width, height));
    float3 InputColor = Input.Sample(InputSampler, UVs);

    float4 color = OutputHDR[ThreadID.xy] + (float4(InputColor, 1.0) * Settings.BloomStrength);
    OutputHDR[ThreadID.xy] = color;
}
