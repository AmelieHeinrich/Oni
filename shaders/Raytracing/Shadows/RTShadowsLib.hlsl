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
    float3 WorldTopLeft;
    float3 WorldTopRight;
    float3 WorldBottomLeft;

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
RaytracingAccelerationStructure Scene : register(t1);

// Special thanks to https://github.com/DeadMG/dx12/blob/master/Renderer.Direct3D12/Shaders/Raytrace/RayGen/Camera.hlsl
void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    ConstantBuffer<Camera> Matrices = ResourceDescriptorHeap[Settings.Camera];

    // Get the location within the dispatched 2D grid of work items
    // (often maps to pixels, so this could represent a pixel coordinate).
    uint2 launchIndex = DispatchRaysIndex().xy;
    // 0.5f puts us in the center of that pixel
    float2 pixelCoordinate = launchIndex.xy + 0.5f;
    // The given dimensions are the absolute maximum of the frustum; they are on the boundaries. There's 1 more boundary than there is pixels.
    float2 dims = float2(DispatchRaysDimensions().xy) + 1.0f;
    
    float3 top = (Matrices.WorldTopRight - Matrices.WorldTopLeft) / dims.x;
    float3 down = (Matrices.WorldBottomLeft - Matrices.WorldTopLeft) / dims.y;
    
    float3 target = Matrices.WorldTopLeft + (top * pixelCoordinate.x) + (down * pixelCoordinate.y);

    origin = Matrices.CameraPosition;
    direction = normalize(target - Matrices.CameraPosition);
}

[shader("raygeneration")]
void RayGeneration()
{
    RWTexture2D<float4> Target = ResourceDescriptorHeap[Settings.Output];

    uint2 index = DispatchRaysIndex().xy;

    float3 origin = 0.0;
    float3 direction = 0.0;
    GenerateCameraRay(index, origin, direction);

    RayDesc ray = (RayDesc)0;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = 1000.0;

    RayPayload payload = (RayPayload)0;
    payload.Missed = false;
    payload.Depth = 1;

    TraceRay(Scene, 0, ~0, 0, 0, 0, ray, payload);

    float3 color = payload.Missed ? 1.0 : 0.5;
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
    ConstantBuffer<LightData> lights = ResourceDescriptorHeap[Settings.Light];

    RayDesc shadowRay = (RayDesc)0;
    shadowRay.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    shadowRay.Direction = normalize(lights.Sun.Direction.xyz);
    shadowRay.TMin = 0.001;
    shadowRay.TMax = 1.0;

    RayPayload shadowPayload = (RayPayload)0;
    shadowPayload.Missed = false;
    shadowPayload.Depth = payload.Depth + 1;

    if (payload.Depth < 2) {
        TraceRay(Scene, RAY_FLAG_NONE, ~0, 0, 0, 0, shadowRay, shadowPayload);
    }
    payload.Missed = shadowPayload.Missed;
}
