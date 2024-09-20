//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 15:24:51
//

struct Parameters
{
    float BloomStrength;
    float3 _Pad0;
};

Texture2D<float3> Input : register(t0);
SamplerState InputSampler : register(s1);

RWTexture2D<float4> OutputHDR : register(u2);
ConstantBuffer<Parameters> Settings : register(b3);

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    uint width, height;
    Input.GetDimensions(width, height);

    float2 UVs = (1.0 / float2(width, height)) * (ThreadID.xy + 0.5);
    float3 InputColor = Input.Sample(InputSampler, UVs);
    float4 HDRColor = OutputHDR[ThreadID.xy];

    OutputHDR[ThreadID.xy] = lerp(HDRColor, float4(InputColor, 1.0), Settings.BloomStrength);
}
