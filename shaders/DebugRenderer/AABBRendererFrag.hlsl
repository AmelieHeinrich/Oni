//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-06 16:39:22
//

struct FragmentIn
{
    float4 Position: SV_POSITION;
};

float4 Main(FragmentIn Input) : SV_Target
{
    return float4(0.0, 1.0, 0.0, 1.0);
};
