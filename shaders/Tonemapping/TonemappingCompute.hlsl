/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 15:44:13
 */

#define TONEMAPPER_ACES 0
#define TONEMAPPER_FILMIC 1
#define TONEMAPPER_RBDH 2

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

struct TonemapperSettings
{
    uint Tonemapper;
    uint InputIndex;
    uint OutputIndex;
    float Gamma;
};

ConstantBuffer<TonemapperSettings> Settings : register(b0);

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    Texture2D HDRTexture = ResourceDescriptorHeap[Settings.InputIndex];
    RWTexture2D<float4> LDRTexture = ResourceDescriptorHeap[Settings.OutputIndex];

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
        float4 final = float4(MappedColor, HDRColor.a);
        final.rgb = pow(final.rgb, 1.0 / Settings.Gamma);

        LDRTexture[ThreadID.xy] = final;
    }
}
