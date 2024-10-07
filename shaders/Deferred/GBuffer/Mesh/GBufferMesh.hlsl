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
    float4 TexCoords : TEXCOORD;
    float4 Normals: NORMAL;
    uint MeshletIndex : COLOR0;
};

struct PushConstants
{
    uint Matrices;
    uint Mesh;
    uint Vertices;
    uint Meshlets;
    uint UniqueVertexIndices;
    uint PrimitiveIndices;
    uint IndexBytes;
    uint MeshletOffset;

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

uint3 UnpackPrimitive(uint primitive)
{
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

uint3 GetPrimitive(Meshlet m, uint index)
{
    StructuredBuffer<uint> PrimitiveIndices = ResourceDescriptorHeap[Constants.PrimitiveIndices];
    // -------- //
    return UnpackPrimitive(PrimitiveIndices[m.PrimOffset + index]);
}

uint GetVertexIndex(Meshlet m, uint localIndex)
{
    ByteAddressBuffer UniqueVertexIndices = ResourceDescriptorHeap[Constants.UniqueVertexIndices];

    // -------- //
    localIndex = m.VertOffset + localIndex;

    if (Constants.IndexBytes == 4) {
        return UniqueVertexIndices.Load(localIndex * 4);
    }
    else // 16-bit Vertex Indices
    {
        // Byte address must be 4-byte aligned.
        uint wordOffset = (localIndex & 0x1);
        uint byteOffset = (localIndex / 2) * 4;

        // Grab the pair of 16-bit indices, shift & mask off proper 16-bits.
        uint indexPair = UniqueVertexIndices.Load(byteOffset);
        uint index = (indexPair >> (wordOffset * 16)) & 0xffff;

        return index;
    }
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
    Output.TexCoords = float4(v.TexCoords, 0.0, 0.0);
    Output.Normals = normalize(float4(mul(Matrices.Transform, float4(v.Normals, 1.0))));
    Output.MeshletIndex = meshletIndex;

    return Output;
}

[numthreads(128, 1, 1)]
[OutputTopology("triangle")]
void Main(
    uint GroupThreadID: SV_GroupThreadID,
    uint GroupID : SV_GroupID,
    out indices uint3 Triangles[126],
    out vertices VertexOut Verts[64]
)
{
    StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[Constants.Meshlets];

    // -------- //
    Meshlet m = Meshlets[Constants.MeshletOffset + GroupID];
    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    if (GroupThreadID < m.PrimCount) {
        Triangles[GroupThreadID] = GetPrimitive(m, GroupThreadID);
    }

    if (GroupThreadID < m.VertCount) {
        uint VertexIndex = GetVertexIndex(m, GroupThreadID);
        Verts[GroupThreadID] = GetVertexAttributes(GroupID, VertexIndex);
    }
}
