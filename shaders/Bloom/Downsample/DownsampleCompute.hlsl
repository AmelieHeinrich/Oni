//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 01:38:51
//

struct Parameters
{
    float2 SrcResolution;
    float2 _Pad0;
};

Texture2D InputHDR : register(t0);
SamplerState InputSampler : register(s1);

RWTexture2D<float4> Downsampled : register(u3);
ConstantBuffer<Parameters> Settings : register(b4);

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    uint width, height;
    Downsampled.GetDimensions(width, height);

    float2 srcTexelSize = 1.0 / Settings.SrcResolution;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    float3 downsample = float3(0.0, 0.0, 0.0);

    if (ThreadID.x < width && ThreadID.y < height)
    {
        float3 a = InputHDR.Sample(InputSampler, float2(ThreadID.x - 2*x, ThreadID.y + 2*y)).rgb;
        float3 b = InputHDR.Sample(InputSampler, float2(ThreadID.x,       ThreadID.y + 2*y)).rgb;
        float3 c = InputHDR.Sample(InputSampler, float2(ThreadID.x + 2*x, ThreadID.y + 2*y)).rgb;

        float3 d = InputHDR.Sample(InputSampler, float2(ThreadID.x - 2*x, ThreadID.y)).rgb;
        float3 e = InputHDR.Sample(InputSampler, float2(ThreadID.x,       ThreadID.y)).rgb;
        float3 f = InputHDR.Sample(InputSampler, float2(ThreadID.x + 2*x, ThreadID.y)).rgb;

        float3 g = InputHDR.Sample(InputSampler, float2(ThreadID.x - 2*x, ThreadID.y - 2*y)).rgb;
        float3 h = InputHDR.Sample(InputSampler, float2(ThreadID.x,       ThreadID.y - 2*y)).rgb;
        float3 i = InputHDR.Sample(InputSampler, float2(ThreadID.x + 2*x, ThreadID.y - 2*y)).rgb;

        float3 j = InputHDR.Sample(InputSampler, float2(ThreadID.x - x,   ThreadID.y + y)).rgb;
        float3 k = InputHDR.Sample(InputSampler, float2(ThreadID.x + x,   ThreadID.y + y)).rgb;
        float3 l = InputHDR.Sample(InputSampler, float2(ThreadID.x - x,   ThreadID.y - y)).rgb;
        float3 m = InputHDR.Sample(InputSampler, float2(ThreadID.x + x,   ThreadID.y - y)).rgb;

        downsample = e * 0.125;
        downsample += (a+c+g+i) * 0.03125;
        downsample += (b+d+f+h) * 0.0625;
        downsample += (j+k+l+m) * 0.125;

        Downsampled[ThreadID.xy] = float4(downsample, 1.0);
    }
}
