//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 01:38:58
//

//
//  https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/
//

struct Parameters
{
    float FilterRadius;
    float3 _Pad0;
};

Texture2D SourceTexture : register(t0);
SamplerState InputSampler : register(s1);

RWTexture2D<float3> Upsampled : register(u2);
ConstantBuffer<Parameters> Settings : register(b3);

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    uint width, height;
    Upsampled.GetDimensions(width, height);

    if (ThreadID.x < width && ThreadID.y < height)
    {
        float x = Settings.FilterRadius;
        float y = Settings.FilterRadius;

        float2 UVs = (1.0 / float2(width, height)) * (ThreadID.xy + 0.5);

        float3 a = SourceTexture.Sample(InputSampler, float2(UVs.x - x, UVs.y + y)).rgb;
        float3 b = SourceTexture.Sample(InputSampler, float2(UVs.x,     UVs.y + y)).rgb;
        float3 c = SourceTexture.Sample(InputSampler, float2(UVs.x + x, UVs.y + y)).rgb;

        float3 d = SourceTexture.Sample(InputSampler, float2(UVs.x - x, UVs.y)).rgb;
        float3 e = SourceTexture.Sample(InputSampler, float2(UVs.x,     UVs.y)).rgb;
        float3 f = SourceTexture.Sample(InputSampler, float2(UVs.x + x, UVs.y)).rgb;

        float3 g = SourceTexture.Sample(InputSampler, float2(UVs.x - x, UVs.y - y)).rgb;
        float3 h = SourceTexture.Sample(InputSampler, float2(UVs.x,     UVs.y - y)).rgb;
        float3 i = SourceTexture.Sample(InputSampler, float2(UVs.x + x, UVs.y - y)).rgb;

        float3 upsample = e * 4.0;
        upsample += (b+d+f+h)*2.0;
        upsample += (a+c+g+i);
        upsample *= 1.0 / 16.0;

        Upsampled[ThreadID.xy] = upsample;
    }
}
