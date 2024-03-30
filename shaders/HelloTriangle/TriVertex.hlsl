/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:52:55
 */

struct VertexIn
{
    float3 Position : POSITION;
};

struct VertexOut
{
    float4 Position : SV_POSITION;
};

VertexOut Main(VertexIn Input)
{
    VertexOut Output = (VertexOut)0;
    Output.Position = float4(Input.Position, 1.0);
    return Output;
}
