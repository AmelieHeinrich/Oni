//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-18 11:50:58
//

#include "shaders/Common/Math.hlsl"

struct ParamsLogC
{
    float cut;
    float a, b, c, d, e, f;
};

static const ParamsLogC LogC =
{
    0.011361, // cut
    5.555556, // a
    0.047996, // b
    0.244161, // c
    0.386036, // d
    5.301883, // e
    0.092819  // f
};

float3 LinearToLogC(float3 x)
{
    return LogC.c * log10(LogC.a * x + LogC.b) + LogC.d;
}

float3 LogCToLinear(float3 x)
{
    return (pow(10.0, (x - LogC.d) / LogC.c) - LogC.b) / LogC.a;
}

float3 RgbToHsv(float3 c)
{
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = Epsilon;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 HsvToRgb(float3 c)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

float RotateHue(float value, float low, float hi)
{
    return (value < low)
            ? value + hi
            : (value > hi)
                ? value - hi
                : value;
}

half Luminance(half3 linearRgb)
{
    return dot(linearRgb, float3(0.2126729, 0.7151522, 0.0721750));
}

half3 SoftLight(half3 backdrop, half3 source)
{
    half3 dependency;
    if (backdrop.r <= 0.25 && backdrop.g <= 0.25 && backdrop.b <= 0.25) dependency = ((16.0 * backdrop.rgb - 12.0) * backdrop.rgb + 4.0) * backdrop.rgb;
    else dependency = sqrt(backdrop.rgb);

    if (backdrop.r <= 0.5 && backdrop.g <= 0.5 && backdrop.b <= 0.5) return backdrop.rgb - (1.0 - 2.0 * source.rgb) * backdrop.rgb * (1.0 - backdrop.rgb);
    else return backdrop.rgb + (2.0 * source.rgb - 1.0) * (dependency - backdrop.rgb);
}
