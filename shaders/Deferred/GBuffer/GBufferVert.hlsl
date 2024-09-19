//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-19 14:05:23
//

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
};

struct SceneData
{
    column_major float4x4 CameraMatrix;
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

    float4 pos = float4(Input.Position, 1.0);
    
    Output.Position = mul(mul(ModelBuffer.Transform, SceneBuffer.CameraMatrix), pos);
    Output.TexCoords = Input.TexCoords;
    Output.Normals = normalize(float4(mul(transpose(ModelBuffer.Transform), float4(Input.Normals, 1.0))).xyz);
    
    return Output;
}
