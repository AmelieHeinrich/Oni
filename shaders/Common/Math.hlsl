//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-18 12:49:33
//

#define PI 3.14159265359
#define TwoPI (2 * PI)
#define Epsilon 0.0001

float radicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float PositivePow(float base, float power)
{
    return pow(max(abs(base), float(Epsilon)), power);
}

float2 PositivePow(float2 base, float2 power)
{
    return pow(max(abs(base), float2(Epsilon, Epsilon)), power);
}

float3 PositivePow(float3 base, float3 power)
{
    return pow(max(abs(base), float3(Epsilon, Epsilon, Epsilon)), power);
}

float4 PositivePow(float4 base, float4 power)
{
    return pow(max(abs(base), float4(Epsilon, Epsilon, Epsilon, Epsilon)), power);
}
