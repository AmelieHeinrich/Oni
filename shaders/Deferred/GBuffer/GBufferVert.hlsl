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
    float4 PrevPosition : POSITION0;
    float4 CurrPosition : POSITION1;
    float4 TexCoords : TEXCOORD;
    float4 Normals: NORMAL;
};

struct ModelMatrices
{
    column_major float4x4 CameraMatrix;
    column_major float4x4 PrevCameraMatrix;
    column_major float4x4 Transform;
    column_major float4x4 PrevTransform;
};

struct SceneData
{
    uint ModelBuffer;
    uint AlbedoTexture;
    uint NormalTexture; 
    uint PBRTexture;
    uint EmissiveTexture;
    uint AOTexture;
    uint Sampler;
    uint _Pad0;
    float2 Jitter;
};

ConstantBuffer<SceneData> Settings : register(b0);

VertexOut Main(VertexIn Input)
{
    ConstantBuffer<ModelMatrices> Matrices = ResourceDescriptorHeap[Settings.ModelBuffer];

    VertexOut Output = (VertexOut)0;

    float4 pos = float4(Input.Position, 1.0);
    
    Output.Position = mul(mul(Matrices.CameraMatrix, Matrices.Transform), pos) + float4(Settings.Jitter, 0.0, 0.0);
    Output.PrevPosition = mul(mul(Matrices.PrevCameraMatrix, Matrices.PrevTransform), pos);
    Output.CurrPosition = mul(mul(Matrices.CameraMatrix, Matrices.Transform), pos);
    Output.TexCoords = float4(Input.TexCoords, 0.0, 0.0);
    Output.Normals = normalize(float4(mul(transpose(Matrices.Transform), float4(Input.Normals, 1.0))));
    
    return Output;
}
