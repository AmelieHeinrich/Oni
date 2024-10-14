// ZPrepassFrag.hlsl

struct FragmentIn
{
    float4 Position : SV_POSITION;
    float4 PrevPosition : POSITION0;
    float4 CurrPosition : POSITION1;
};

float2 Main(FragmentIn Input) : SV_Target
{
    float2 oldPos = Input.PrevPosition.xy / Input.PrevPosition.w;
    float2 newPos = Input.CurrPosition.xy / Input.CurrPosition.w;
    float2 positionDifference = (oldPos - newPos);
    positionDifference *= float2(-1.0f, 1.0f);

    return positionDifference;
}
