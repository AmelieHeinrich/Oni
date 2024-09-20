//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 01:38:51
//

//
//  https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/
//

struct Parameters
{
    float2 SrcResolution;
    float2 _Pad0;
};

Texture2D InputHDR : register(t0);
SamplerState InputSampler : register(s1);

RWTexture2D<float3> Downsampled : register(u2);
ConstantBuffer<Parameters> Settings : register(b3);

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    uint width, height;
    Downsampled.GetDimensions(width, height);

    float2 srcTexelSize = 1.0 / Settings.SrcResolution;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    float2 UVs = (1.0 / float2(width, height)) * (ThreadID.xy + 0.5);

    float3 a = InputHDR.Sample(InputSampler, float2(UVs.x - 2*x, UVs.y + 2*y)).rgb;
    float3 b = InputHDR.Sample(InputSampler, float2(UVs.x,       UVs.y + 2*y)).rgb;
    float3 c = InputHDR.Sample(InputSampler, float2(UVs.x + 2*x, UVs.y + 2*y)).rgb;

    float3 d = InputHDR.Sample(InputSampler, float2(UVs.x - 2*x, UVs.y)).rgb;
    float3 e = InputHDR.Sample(InputSampler, float2(UVs.x,       UVs.y)).rgb;
    float3 f = InputHDR.Sample(InputSampler, float2(UVs.x + 2*x, UVs.y)).rgb;

    float3 g = InputHDR.Sample(InputSampler, float2(UVs.x - 2*x, UVs.y - 2*y)).rgb;
    float3 h = InputHDR.Sample(InputSampler, float2(UVs.x,       UVs.y - 2*y)).rgb;
    float3 i = InputHDR.Sample(InputSampler, float2(UVs.x + 2*x, UVs.y - 2*y)).rgb;

    float3 j = InputHDR.Sample(InputSampler, float2(UVs.x - x,   UVs.y + y)).rgb;
    float3 k = InputHDR.Sample(InputSampler, float2(UVs.x + x,   UVs.y + y)).rgb;
    float3 l = InputHDR.Sample(InputSampler, float2(UVs.x - x,   UVs.y - y)).rgb;
    float3 m = InputHDR.Sample(InputSampler, float2(UVs.x + x,   UVs.y - y)).rgb;

    float3 downsample = e * 0.125;
    downsample += (a+c+g+i) * 0.03125;
    downsample += (b+d+f+h) * 0.0625;
    downsample += (j+k+l+m) * 0.125;

    Downsampled[ThreadID.xy] = downsample;
}
