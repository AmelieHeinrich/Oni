/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:44:13
 */

#define TONEMAPPER_ACES 0
#define TONEMAPPER_FILMIC 1
#define TONEMAPPER_RBDH 2

struct TonemapperSettings
{
    int Tonemapper;
    uint _Padding0;
    uint _Padding1;
    uint _Padding2;
};

float3 ACESFilm(float3 X)
{
    float A = 2.51f;
    float B = 0.03f;
    float C = 2.43f;
    float D = 0.59f;
    float E = 0.14f;
    return saturate((X * (A * X + B)) / (X * (C * X + D) + E));
}

float3 Filmic(float3 X)
{
    X = max(float3(0.0, 0.0, 0.0), X - float3(0.004, 0.004, 0.004));
    X = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
    return X;
}

float3 RomBinDaHouse(float3 X)
{
    X = exp(-1.0 / (2.72 * X + 0.15));
	return X;
}

Texture2D HDRTexture : register(t0);
RWTexture2D<float4> LDRTexture : register(u1);
ConstantBuffer<TonemapperSettings> Settings : register(b2);

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    uint Width, Height;
    HDRTexture.GetDimensions(Width, Height);

    if (ThreadID.x <= Width && ThreadID.y <= Height)
    {
        float4 HDRColor = HDRTexture[ThreadID.xy];
        float3 MappedColor = HDRColor.xyz;
        switch (Settings.Tonemapper)
        {
            case TONEMAPPER_ACES: {
                MappedColor = ACESFilm(HDRColor.xyz);
                break;
            }
            case TONEMAPPER_FILMIC: {
                MappedColor = Filmic(HDRColor.xyz);
                break;
            }
            case TONEMAPPER_RBDH: {
                MappedColor = RomBinDaHouse(HDRColor.xyz);
                break;
            }
        }
        LDRTexture[ThreadID.xy] = float4(MappedColor, HDRColor.a);
    }
}
