//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-04-09 23:07:22
//

#include "shaders/Common/Colors.hlsl"

#define ACEScc_MIDGRAY 0.4135884

struct RendererSettings
{
    float Exposure;
    float3 _Pad;
    float Contrast;
    float3 _Pad1;
    float4 ColorFilter;
    float HueShift;
    float Saturation;
    float2 _Pad2;
};

RWTexture2D<float4> Texture : register(u0);
ConstantBuffer<RendererSettings> Settings : register(b1);

float3 ColorGradePostExposure(float3 color)
{
	return color * pow(2.0, Settings.Exposure);
}

float3 ColorGradingContrast(float3 color)
{
    color = LinearToLogC(color);
	color = (color - ACEScc_MIDGRAY) * (Settings.Contrast * 0.01 + 1.0) + ACEScc_MIDGRAY;
	return LogCToLinear(color);
}

float3 ColorGradeColorFilter(float3 color)
{
	return color * Settings.ColorFilter.rgb;
}

float3 ColorGradingHueShift(float3 color)
{
	color = RgbToHsv(color);
	float hue = color.x + (Settings.HueShift * (1.0 / 360.0));
	color.x = RotateHue(hue, 0.0, 1.0);
	return HsvToRgb(color);
}

float3 ColorGradingSaturation(float3 color)
{
	float luminance = Luminance(color);
	return (color - luminance) * (Settings.Saturation * 0.1 + 1.0) + luminance;
}

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    uint Width, Height;
    Texture.GetDimensions(Width, Height);

    if (ThreadID.x < Width && ThreadID.y < Height)
    {
        float3 BaseColor = Texture[ThreadID.xy].xyz;
        float4 ColorAdjustments = float4(
            pow(2, Settings.Exposure),
            Settings.Contrast * 0.01 + 1.0,
            Settings.HueShift * (1.0 / 360.0),
            Settings.Saturation * 0.1 + 1.0
        );

        BaseColor = min(BaseColor, 60.0);
        BaseColor = ColorGradePostExposure(BaseColor);
        BaseColor = ColorGradingContrast(BaseColor);
        BaseColor = ColorGradeColorFilter(BaseColor);
        BaseColor = max(BaseColor, 0.0);
        BaseColor = ColorGradingHueShift(BaseColor);
        BaseColor = ColorGradingSaturation(BaseColor);
        BaseColor = max(BaseColor, 0.0);

        Texture[ThreadID.xy] = float4(BaseColor, 1.0f);
    }
}