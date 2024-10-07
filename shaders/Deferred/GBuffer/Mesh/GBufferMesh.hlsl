//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-07 19:15:54
//

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

[numthreads(128, 1, 1)]
[OutputTopology("triangle")]
void Main(
    uint GroupThreadID: SV_GroupThreadID,
    uint GroupID : SV_GroupID,
    out indices uint3 Triangles[124],
    out vertices VertexOut Verts[48]
)
{
    StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[Constants.Meshlets];
    StructuredBuffer<uint32_t> Indices = ResourceDescriptorHeap[Constants.UniqueVertexIndices];
    StructuredBuffer<uint32_t> MeshletTriangles = ResourceDescriptorHeap[Constants.Triangles];

    // -------- //
    Meshlet m = Meshlets[GroupID];
    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    if (GroupThreadID < m.PrimCount) {
        uint packed = MeshletTriangles[m.PrimOffset + GroupThreadID];
        uint vIdx0 = (packed >>  0) & 0xFF;
        uint vIdx1 = (packed >>  8) & 0xFF;
        uint vIdx2 = (packed >> 16) & 0xFF;
        Triangles[GroupThreadID] = uint3(vIdx0, vIdx1, vIdx2);
    }

    if (GroupThreadID < m.VertCount) {
        uint vertexIndex = m.VertOffset + GroupThreadID;
        vertexIndex = Indices[vertexIndex];

        Verts[GroupThreadID] = GetVertexAttributes(GroupThreadID, vertexIndex);
    }
}
