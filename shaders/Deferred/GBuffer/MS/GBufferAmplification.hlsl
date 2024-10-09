//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-09 21:51:35
//

struct Payload
{
    uint MeshletIndices[32];
};

struct MeshletBounds
{
    /* bounding sphere, useful for frustum and occlusion culling */
	float3 center;
	float radius;

	/* normal cone, useful for backface culling */
	float3 cone_apex;
	float3 cone_axis;
	float cone_cutoff; /* = cos(angle/2) */
};

struct ModelMatrices
{
    column_major float4x4 CameraMatrix;
    column_major float4x4 PrevCameraMatrix;
    column_major float4x4 Transform;
    column_major float4x4 PrevTransform;

    float3 CameraPosition;
    float Scale;
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

    uint AlbedoTexture;
    uint NormalTexture;
    uint PBRTexture;
    uint EmissiveTexture;
    uint AOTexture;
    uint Sampler;
    
    uint DrawMeshlets;
    float EmissiveStrength;
    float2 Jitter;
    uint Pad;
};

ConstantBuffer<PushConstants> Constants : register(b0);

groupshared Payload s_Payload;

bool IsVisible(MeshletBounds bound)
{
    ConstantBuffer<ModelMatrices> Matrices = ResourceDescriptorHeap[Constants.Matrices];
    
    float4 center = mul(float4(bound.center.xyz, 1), Matrices.Transform);
    
    float3 axis = normalize(mul(float4(bound.cone_axis.xyz, 0), Matrices.Transform)).xyz;
    float3 apex = center.xyz - axis * bound.cone_apex;
    float3 view = normalize(Matrices.CameraPosition - apex);

    if (dot(view, -axis) > bound.cone_cutoff)
        return true;
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
