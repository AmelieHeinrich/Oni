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
    float3 Normals: NORMAL;
    float4 WorldPos : COLOR0;
    float4 CameraPosition: COLOR1;
    float4 LightPos: COLOR2;
};

struct SceneData
{
    column_major float4x4 View;
    column_major float4x4 Projection;
    float4 CameraPosition;
    column_major float4x4 SunMatrix;
};

struct ModelData
{
    column_major float4x4 Transform;
    float4 FlatColor;
};

ConstantBuffer<SceneData> SceneBuffer : register(b0);
ConstantBuffer<ModelData> ModelBuffer : register(b1);

VertexOut Main(VertexIn Input)
{
    VertexOut Output = (VertexOut)0;
    
    Output.Position = mul(float4(Input.Position, 1.0f), ModelBuffer.Transform);
    Output.Position = mul(SceneBuffer.View, Output.Position);
    Output.Position = mul(SceneBuffer.Projection, Output.Position);

    Output.TexCoords = Input.TexCoords;
    
    Output.Normals = normalize(float4(mul(transpose(ModelBuffer.Transform), float4(Input.Normals, 1.0))).xyz);
    
    Output.WorldPos = mul(float4(Input.Position, 1.0), ModelBuffer.Transform);
    
    Output.CameraPosition = SceneBuffer.CameraPosition;
    
    Output.LightPos = mul(float4(Input.Position, 1.0f), ModelBuffer.Transform);
    Output.LightPos = mul(SceneBuffer.SunMatrix, Output.LightPos);
    
    return Output;
}
