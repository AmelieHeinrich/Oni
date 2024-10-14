// Mesh.hlsl

struct Meshlet
{
    uint VertOffset;
    uint PrimOffset;
    uint VertCount;
    uint PrimCount;
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
