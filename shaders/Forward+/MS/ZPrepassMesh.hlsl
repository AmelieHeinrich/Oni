//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-07 19:15:54
//

#include "shaders/Common/Mesh.hlsl"

struct Vertex
{
    float3 Position : POSITION;
    float2 TexCoords : TEXCOORD;
    float3 Normals : NORMAL;
};

struct ModelMatrices
{
    column_major float4x4 CameraMatrix;
    column_major float4x4 PrevCameraMatrix;
    column_major float4x4 Transform;
    column_major float4x4 PrevTransform;

    float3 CameraPosition;
};

struct VertexOut
{
    float4 Position : SV_POSITION;
    float4 PrevPosition : POSITION0;
    float4 CurrPosition : POSITION1;
};

struct PushConstants
{
    uint Matrices;
    uint VertexBuffer;
    uint IndexBuffer;
    uint MeshletBuffer;
    uint MeshletVertices;
    uint MeshletTriangleBuffer;
    uint MeshletBoundsBuffer;

    float2 Jitter;
};

struct Payload
{
    uint MeshletIndices[32];
};

ConstantBuffer<PushConstants> Constants : register(b0);

VertexOut GetVertexAttributes(uint meshletIndex, uint vertexIndex)
{
    StructuredBuffer<Vertex> Vertices = ResourceDescriptorHeap[Constants.VertexBuffer];
    ConstantBuffer<ModelMatrices> Matrices = ResourceDescriptorHeap[Constants.Matrices];

    // -------- //
    Vertex v = Vertices[vertexIndex];
    float4 pos = float4(v.Position, 1.0);

    VertexOut Output = (VertexOut)0;
    Output.Position = mul(mul(Matrices.CameraMatrix, Matrices.Transform), pos) + float4(Constants.Jitter, 0.0, 0.0);
    Output.PrevPosition = mul(mul(Matrices.PrevCameraMatrix, Matrices.PrevTransform), pos);
    Output.CurrPosition = mul(mul(Matrices.CameraMatrix, Matrices.Transform), pos);

    return Output;
}

[numthreads(32, 1, 1)]
[OutputTopology("triangle")]
void Main(
    uint GroupThreadID: SV_GroupThreadID,
    uint GroupID : SV_GroupID,
    in payload Payload payload,
    out indices uint3 Triangles[124],
    out vertices VertexOut Verts[64]
)
{
    StructuredBuffer<uint> Indices = ResourceDescriptorHeap[Constants.IndexBuffer];
    StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[Constants.MeshletBuffer];
    StructuredBuffer<uint> MeshletVertices = ResourceDescriptorHeap[Constants.MeshletVertices];
    StructuredBuffer<uint> MeshletPrimitives = ResourceDescriptorHeap[Constants.MeshletTriangleBuffer];

    // -------- //
    uint meshletIndex = payload.MeshletIndices[GroupID];
    Meshlet m = Meshlets[meshletIndex];

    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    for (uint i = GroupThreadID; i < m.PrimCount; i += 32) {
        uint offset = m.PrimOffset + i * 3;
        Triangles[i] = uint3(
            MeshletPrimitives[offset],
            MeshletPrimitives[offset + 1],
            MeshletPrimitives[offset + 2]
        );
    }

    for (uint i = GroupThreadID; i < m.VertCount; i += 32) {
        uint index = MeshletVertices[m.VertOffset + i];
        Verts[i] = GetVertexAttributes(GroupID, index);
    }
}
