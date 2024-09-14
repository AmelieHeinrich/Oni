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

struct ShadowParameters
{
    column_major float4x4 LightSpaceMatrix;
    column_major float4x4 ModelMatrix;
};

ConstantBuffer<ShadowParameters> Params : register(b0);

VertexOut Main(VertexIn input)
{
    VertexOut output = (VertexOut)0;

    output.Position = mul(float4(input.Position, 1.0), Params.ModelMatrix);
    output.Position = mul(Params.LightSpaceMatrix, output.Position);
    
    return output;
}
