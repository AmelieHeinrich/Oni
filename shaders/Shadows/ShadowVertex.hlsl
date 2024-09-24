/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-07-15 03:19:13
 */

struct VertexIn
{
    float3 Position : POSITION;
};

struct VertexOut
{
    float4 Position : SV_POSITION;
};

struct PushConstants
{
    column_major float4x4 SunMatrix;
    column_major float4x4 ModelMatrix;
};

ConstantBuffer<PushConstants> Constants : register(b0);

VertexOut Main(VertexIn input)
{
    VertexOut output = (VertexOut)0;

    output.Position = mul(mul(Constants.SunMatrix, Constants.ModelMatrix), float4(input.Position, 1.0));
    
    return output;
}
