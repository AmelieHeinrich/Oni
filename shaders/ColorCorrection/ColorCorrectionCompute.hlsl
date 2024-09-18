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
    float Temperature;
    float Tint;

    float4 Shadows;
    float4 Highlights;
    float Balance;
    
    float3 _Pad2;
};

RWTexture2D<float4> Texture : register(u0);
ConstantBuffer<RendererSettings> Settings : register(b1);

float3 ColorGradeWhiteBalance(float3 col) 
{
    float t1 = Settings.Temperature * 10.0f / 6.0f;
    float t2 = Settings.Tint * 10.0f / 6.0f;

    float x = 0.31271 - t1 * (t1 < 0 ? 0.1 : 0.05);
    float standardIlluminantY = 2.87 * x - 3 * x * x - 0.27509507;
    float y = standardIlluminantY + t2 * 0.05;

    float3 w1 = float3(0.949237, 1.03542, 1.08728);

    float Y = 1;
    float X = Y * x / y;
    float Z = Y * (1 - x - y) / y;
    float L = 0.7328 * X + 0.4296 * Y - 0.1624 * Z;
    float M = -0.7036 * X + 1.6975 * Y + 0.0061 * Z;
    float S = 0.0030 * X + 0.0136 * Y + 0.9834 * Z;
    float3 w2 = float3(L, M, S);

    float3 balance = float3(w1.x / w2.x, w1.y / w2.y, w1.z / w2.z);

    float3x3 LIN_2_LMS_MAT = float3x3(
        float3(3.90405e-1, 5.49941e-1, 8.92632e-3),
        float3(7.08416e-2, 9.63172e-1, 1.35775e-3),
        float3(2.31082e-2, 1.28021e-1, 9.36245e-1)
    );

    float3x3 LMS_2_LIN_MAT = float3x3(
        float3(2.85847e+0, -1.62879e+0, -2.48910e-2),
        float3(-2.10182e-1,  1.15820e+0,  3.24281e-4),
        float3(-4.18120e-2, -1.18169e-1,  1.06867e+0)
    );

    float3 lms = mul(LIN_2_LMS_MAT, col);
    lms *= balance;

    return mul(LMS_2_LIN_MAT, lms);
}

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

float3 ColorGradeSplitToning(float3 color)
{
	color = PositivePow(color, 1.0 / 2.2);
	float t = saturate(Luminance(saturate(color)) + Settings.Shadows.w);
	float3 shadows = lerp(0.5, Settings.Shadows.rgb, 1.0 - t);
	float3 highlights = lerp(0.5, Settings.Highlights.rgb, t);
	color = SoftLight(color, shadows);
	color = SoftLight(color, highlights);
	return PositivePow(color, 2.2);
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
        BaseColor = ColorGradeWhiteBalance(BaseColor);
        BaseColor = ColorGradingContrast(BaseColor);
        BaseColor = ColorGradeColorFilter(BaseColor);
        BaseColor = max(BaseColor, 0.0);
        BaseColor = ColorGradeSplitToning(BaseColor);
        BaseColor = ColorGradingHueShift(BaseColor);
        BaseColor = ColorGradingSaturation(BaseColor);
        BaseColor = max(BaseColor, 0.0);

        Texture[ThreadID.xy] = float4(BaseColor, 1.0f);
    }
}