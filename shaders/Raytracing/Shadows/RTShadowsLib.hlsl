//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-10 16:32:45
//

#include "shaders/Common/Lights.hlsl"

struct RayPayload
{
    bool Missed;
    float Depth;
};

struct Camera
{
    column_major float4x4 CameraMatrix;
    float3 CameraPosition;
    float Pad;
};

struct Constants
{
    uint Scene;
    uint Camera;
    uint Light;
    uint Output;
};

ConstantBuffer<Constants> Settings : register(b0);

void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    ConstantBuffer<Camera> Matrices = ResourceDescriptorHeap[Settings.Camera];

    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(Matrices.CameraMatrix, float4(screenPos, 0, 1));
    world.xyz /= world.w;

    origin = Matrices.CameraPosition;
    direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void RayGeneration()
{
    RaytracingAccelerationStructure scene = ResourceDescriptorHeap[Settings.Scene];
    RWTexture2D<float4> Target = ResourceDescriptorHeap[Settings.Output];

    uint2 index = DispatchRaysIndex().xy;

    float3 origin = 0.0;
    float3 direction = 0.0;
    GenerateCameraRay(index, origin, direction);

    RayDesc ray = (RayDesc)0;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    RayPayload payload = (RayPayload)0;
    payload.Missed = false;
    payload.Depth = 0;

    TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);

    float3 color = payload.Missed ? 0.5 : 1.0;
    Target[index] = float4(color, 1.0);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.Missed = true;
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    RaytracingAccelerationStructure scene = ResourceDescriptorHeap[Settings.Scene];
    ConstantBuffer<LightData> lights = ResourceDescriptorHeap[Settings.Light];

    float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    RayDesc shadowRay = (RayDesc)0;
    shadowRay.Origin = pos;
    shadowRay.Direction = -lights.Sun.Direction.xyz;
    shadowRay.TMin = 0.001;
    shadowRay.TMax = 1;

    RayPayload shadowPayload = (RayPayload)0;
    shadowPayload.Missed = false;
    shadowPayload.Depth = 0;

    if (payload.Depth < 2) {
        TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, shadowRay, shadowPayload);
        payload.Depth++;
    }
}
