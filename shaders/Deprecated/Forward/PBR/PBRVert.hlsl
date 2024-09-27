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
    column_major float4x4 CameraMatrix;
    column_major float4x4 SunMatrix;
    float4 CameraPosition;
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
    Output.LightPos = mul(mul(ModelBuffer.Transform, SceneBuffer.SunMatrix), pos);
    Output.WorldPos = mul(pos, ModelBuffer.Transform);

    Output.TexCoords = Input.TexCoords;
    Output.Normals = normalize(float4(mul(transpose(ModelBuffer.Transform), float4(Input.Normals, 1.0))).xyz);
    Output.CameraPosition = SceneBuffer.CameraPosition;
    
    return Output;
}
