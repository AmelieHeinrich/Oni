/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:52:55
 */

struct VertexIn
{
    float3 Position : POSITION;
    float2 TexCoords : TEXCOORD;
    float3 Normals : NORMAL;
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

struct ModelData
{
    row_major float4x4 Transform;
};

ConstantBuffer<SceneData> SceneBuffer : register(b0);
ConstantBuffer<ModelData> ModelBuffer : register(b1);

VertexOut Main(VertexIn Input)
{
    VertexOut Output = (VertexOut)0;
    Output.Position = mul(float4(Input.Position, 1.0f), ModelBuffer.Transform);
    Output.Position = mul(Output.Position, SceneBuffer.View);
    Output.Position = mul(Output.Position, SceneBuffer.Projection);
    Output.TexCoords = Input.TexCoords;
    return Output;
}