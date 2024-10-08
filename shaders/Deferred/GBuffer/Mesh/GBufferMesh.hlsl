//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-07 19:15:54
//

#define DEBUG_TRIANGLE

struct Vertex
{
    float3 Position : POSITION;
    float2 TexCoords : TEXCOORD;
    float3 Normals : NORMAL;
};

struct Meshlet
{
    uint VertOffset;
    uint PrimOffset;
    uint VertCount;
    uint PrimCount;
};

struct ModelMatrices
{
    column_major float4x4 CameraMatrix;
    column_major float4x4 PrevCameraMatrix;
    column_major float4x4 Transform;
    column_major float4x4 PrevTransform;
};

struct VertexOut
{
    float4 Position : SV_POSITION;
    float4 PrevPosition : POSITION0;
    float4 CurrPosition : POSITION1;
    float3 Normals: NORMAL;
    uint MeshletIndex : COLOR0;
    float2 TexCoords : TEXCOORD;
    float2 Pad : POSITION2;
};

struct PushConstants
{
    uint Matrices;
    uint Vertices;
    uint UniqueVertexIndices;
    uint Meshlets;
    uint Triangles;

    uint AlbedoTexture;
    uint NormalTexture;
    uint PBRTexture;
    uint EmissiveTexture;
    uint AOTexture;
    uint Sampler;
    
    uint DrawMeshlets;
    float EmissiveStrength;
    float2 Jitter;
    float Pad;
};

ConstantBuffer<PushConstants> Constants : register(b0);

VertexOut GetVertexAttributes(uint meshletIndex, uint vertexIndex)
{
    StructuredBuffer<Vertex> Vertices = ResourceDescriptorHeap[Constants.Vertices];
    ConstantBuffer<ModelMatrices> Matrices = ResourceDescriptorHeap[Constants.Matrices];

    // -------- //
    Vertex v = Vertices[vertexIndex];
    float4 pos = float4(v.Position, 1.0);

    VertexOut Output = (VertexOut)0;
    Output.Position = mul(mul(Matrices.CameraMatrix, Matrices.Transform), pos) + float4(Constants.Jitter, 0.0, 0.0);
    Output.PrevPosition = mul(mul(Matrices.PrevCameraMatrix, Matrices.PrevTransform), pos);
    Output.CurrPosition = mul(mul(Matrices.CameraMatrix, Matrices.Transform), pos);
    Output.TexCoords = v.TexCoords;
    Output.Normals = normalize(float4(mul(Matrices.Transform, float4(v.Normals, 1.0)))).xyz;
    Output.MeshletIndex = meshletIndex;

    return Output;
}

[numthreads(32, 1, 1)]
[OutputTopology("triangle")]
void Main(
    uint GroupThreadID: SV_GroupThreadID,
    uint GroupID : SV_GroupID,
    out indices uint3 Triangles[124],
    out vertices VertexOut Verts[64]
)
{
    StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[Constants.Meshlets];
    StructuredBuffer<uint> Indices = ResourceDescriptorHeap[Constants.UniqueVertexIndices];
    StructuredBuffer<uint> MeshletPrimitives = ResourceDescriptorHeap[Constants.Triangles];

    // -------- //
    Meshlet m = Meshlets[GroupID];
    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    uint indexProcessingIterations = (m.PrimCount + 32 - 1) / 32;
    for (int i = 0; i < indexProcessingIterations; i++) {
        if (GroupThreadID * indexProcessingIterations + i >= m.PrimCount) {
            break;
        }

        Triangles[GroupThreadID * indexProcessingIterations + i] = uint3(
            MeshletPrimitives[m.PrimOffset + ((GroupThreadID * indexProcessingIterations + i) * 3)],
            MeshletPrimitives[m.PrimOffset + ((GroupThreadID * indexProcessingIterations + i) * 3 + 1)],
            MeshletPrimitives[m.PrimOffset + ((GroupThreadID * indexProcessingIterations + i) * 3 + 2)]
        );
    }

    uint vertexProcessingIterations = (m.VertCount + 32 - 1) / 32;
    for (int i = 0; i < vertexProcessingIterations; i++) {
        if ((GroupThreadID * vertexProcessingIterations + i) >= m.VertCount) {
            break;
        }

        uint vertexIndex = GroupThreadID * vertexProcessingIterations + (i * 3);
        Verts[GroupThreadID * vertexProcessingIterations + i] = GetVertexAttributes(GroupThreadID, vertexIndex);
    }
}
