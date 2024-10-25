//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-14 15:07:35
//

#include "shaders/Common/Mesh.hlsl"

struct Payload
{
    uint MeshletIndices[32];
};

struct ModelMatrices
{
    column_major float4x4 CameraMatrix;
    column_major float4x4 PrevCameraMatrix;
    column_major float4x4 Transform;
    column_major float4x4 PrevTransform;

    float3 CameraPosition;
    float Scale;

    float4 Planes[6];    
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

ConstantBuffer<PushConstants> Constants : register(b0);

groupshared Payload s_Payload;

bool IsVisible(MeshletBounds bound)
{
    ConstantBuffer<ModelMatrices> Matrices = ResourceDescriptorHeap[Constants.Matrices];
    
    float4 center = mul(Matrices.Transform, float4(bound.center.xyz, 1));
    float radius = bound.radius * Matrices.Scale;

    float3 axis = normalize(mul(Matrices.Transform, float4(bound.cone_axis.xyz, 0))).xyz;
    float3 apex = center.xyz - axis * bound.cone_apex;
    float3 view = normalize(Matrices.CameraPosition - apex);

    // CONE CULLING: FIX THAT SHIT BRO
    //if (dot(view, -axis) > bound.cone_cutoff)
    //    return true;
    
    // FRUSTUM CULLING
    for (int i = 0; i < 6; i++) {
        if (dot(Matrices.Planes[i].xyz, center.xyz) - Matrices.Planes[i].w <= -radius)
            return false;
    }

    return true;
}

[numthreads(32, 1, 1)]
void Main(uint gtid : SV_GroupThreadID, uint dtid : SV_DispatchThreadID, uint gid : SV_GroupID)
{
    StructuredBuffer<MeshletBounds> bounds = ResourceDescriptorHeap[Constants.MeshletBoundsBuffer];
    
    bool visible = IsVisible(bounds[dtid]);
    if (visible) {
        uint index = WavePrefixCountBits(visible);
        s_Payload.MeshletIndices[index] = dtid;
    }

    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, s_Payload);
}
