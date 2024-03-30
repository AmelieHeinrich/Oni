/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:53:11
 */

struct FragmentIn
{
    float4 Position : SV_POSITION;
};

float4 Main(FragmentIn Input) : SV_TARGET
{
    return float4(1.0, 1.0, 1.0, 1.0);
}