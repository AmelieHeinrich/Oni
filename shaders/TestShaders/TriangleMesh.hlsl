//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-08 17:40:27
//

struct VertexIn
{
    float3 Position : POSITION;
    float3 Color : COLOR;
};

struct MeshOutput
{
    float4 Position : SV_POSITION;
    float3 Color    : COLOR;
};

struct Meshlet
{
    uint VertOffset;
    uint PrimOffset;
    uint VertCount;
    uint PrimCount;
};

struct Placeholder
{
    uint VertexBuffer;
    uint IndexBuffer;
    uint MeshletBuffer;
    uint MeshletTriangleBuffer;
};

ConstantBuffer<Placeholder> Placeholder : register(b0);

[outputtopology("triangle")]
[numthreads(32, 1, 1)]
void Main(uint GroupID : SV_GroupID,
          uint GroupThreadID : SV_GroupThreadID,
          out indices uint3 triangles[4],
          out vertices MeshOutput vertices[12]) {
    StructuredBuffer<VertexIn> Vertices = ResourceDescriptorHeap[Placeholder.VertexBuffer];
    StructuredBuffer<uint> Indices = ResourceDescriptorHeap[Placeholder.IndexBuffer];
    StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[Placeholder.MeshletBuffer];
    StructuredBuffer<uint> MeshletPrimitives = ResourceDescriptorHeap[Placeholder.MeshletTriangleBuffer];

    //
    Meshlet m = Meshlets[GroupID];
    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    // Vertices
    uint vertexProcessingIterations = (m.VertCount + 32 - 1) / 32;
    for (int i = 0; i < vertexProcessingIterations; i++) {
        if ((GroupThreadID * vertexProcessingIterations + i) >= m.VertCount) {
            break;
        }

        uint vertexIndex = GroupThreadID * vertexProcessingIterations + (i * 3);
        VertexIn vertex = Vertices[vertexIndex];

        vertices[GroupThreadID * vertexProcessingIterations + i].Position = float4(vertex.Position, 1.0);
        vertices[GroupThreadID * vertexProcessingIterations + i].Color = vertex.Color;
    }

    // Triangles
    uint indexProcessingIterations = (m.PrimCount + 32 - 1) / 32;
    for (int i = 0; i < indexProcessingIterations; i++) {
        if (GroupThreadID * indexProcessingIterations + i >= m.PrimCount) {
            break;
        }

        triangles[GroupThreadID * indexProcessingIterations + i] = uint3(
            MeshletPrimitives[m.PrimOffset + ((GroupThreadID * indexProcessingIterations + i) * 3)],
            MeshletPrimitives[m.PrimOffset + ((GroupThreadID * indexProcessingIterations + i) * 3 + 1)],
            MeshletPrimitives[m.PrimOffset + ((GroupThreadID * indexProcessingIterations + i) * 3 + 2)]
        );
    }
}
