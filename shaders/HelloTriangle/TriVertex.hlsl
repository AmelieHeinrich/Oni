/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:52:55
 */

struct VertexIn
{
    float3 Position : POSITION;
    float2 TexCoords : TEXCOORD;
};

struct VertexOut
{
    float4 Position : SV_POSITION;
    float2 TexCoords : TEXCOORD;
};

struct SceneData
{
    row_major float4x4 View;
    row_major float4x4 Projection;
};

ConstantBuffer<SceneData> SceneBuffer : register(b0);

VertexOut Main(VertexIn Input)
{
    VertexOut Output = (VertexOut)0;
    Output.Position = mul(float4(Input.Position, 1.0f), SceneBuffer.View);
    Output.Position = mul(Output.Position, SceneBuffer.Projection);
    Output.TexCoords = Input.TexCoords;
    return Output;
}
