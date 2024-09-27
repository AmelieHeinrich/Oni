//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-15 10:50:51
//

struct FragmentIn
{
    float4 Position: SV_POSITION;
    float4 Col: COLOR;
};

float4 Main(FragmentIn Input) : SV_Target
{
    return Input.Col;
};
