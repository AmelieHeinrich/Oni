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
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
};

struct MeshletTriangle
{
    uint32_t V0;
	uint32_t V1;
	uint32_t V2;
	uint32_t Pad;
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
};

ConstantBuffer<PushConstants> Constants : register(b0);

uint GetVertexIndex(Meshlet m, uint localIndex)
{
    ByteAddressBuffer UniqueVertexIndices = ResourceDescriptorHeap[Constants.UniqueVertexIndices];

    // -------- //
    localIndex = m.VertOffset + localIndex;
    return UniqueVertexIndices.Load(localIndex * 4);
}

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
    out vertices VertexOut Verts[64]
)
{
    StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[Constants.Meshlets];
    StructuredBuffer<MeshletTriangle> MeshletTriangles = ResourceDescriptorHeap[Constants.Triangles];

    // -------- //
    Meshlet m = Meshlets[GroupID];
    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    if (GroupThreadID < m.PrimCount) {
        Triangles[GroupThreadID] = uint3(MeshletTriangles[m.PrimOffset + GroupThreadID].V0,
                                         MeshletTriangles[m.PrimOffset + GroupThreadID].V1,
                                         MeshletTriangles[m.PrimOffset + GroupThreadID].V2);
    }

    if (GroupThreadID < m.VertCount) {
        uint VertexIndex = GetVertexIndex(m, GroupThreadID);
        Verts[GroupThreadID] = GetVertexAttributes(GroupID, VertexIndex);
    }
}
