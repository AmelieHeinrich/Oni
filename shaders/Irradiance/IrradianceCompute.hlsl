/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 00:39:25
 */

#define PI 3.14159265359

TextureCube EnvironmentMap : register(t0);
RWTexture2DArray<half4> IrradianceMap : register(u1);
SamplerState CubeSampler : register(s2);

static const float3 ViewPos = float3(0, 0, 0);

static const float3 ViewTarget[6] = {
	float3(1, 0, 0),
    float3(-1, 0, 0),
    float3(0, -1, 0),
    float3(0, 1, 0),
    float3(0, 0, 1),
    float3(0, 0, -1)
};

static const float3 ViewUp[6] = {
    float3(0, 1, 0),
    float3(0, 1, 0),
    float3(0, 0, 1), // +Y
    float3(0, 0, -1), // -Y
    float3(0, 1, 0),
    float3(0, 1, 0)
};

float4x4 LookAt(float3 pos, float3 target, float3 world_up) {
    float3 fwd = normalize(target - pos);
    float3 right = cross(world_up, fwd);
    float3 up = cross(fwd, right);
    return float4x4(float4(right, 0), float4(up, 0), float4(fwd, 0), float4(pos, 1));
}

float2 UVToQuadPos(float2 uv) {
    return uv * 2.0f - 1.0f;
}

float3 ToLocalDirection(float2 uv, int face) {
    float4x4 view = LookAt(ViewPos, ViewTarget[face], ViewUp[face]);
    return (mul(view, float4(UVToQuadPos(uv), 1, 0))).xyz;
}

float3 calculate_irradiance(float3 local_direction) {
    float3 normal = normalize(local_direction);
    float3 irradiance = float3(0.0, 0.0, 0.0);

    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, normal));
    up = cross(normal, right);

    float sample_delta = 0.025;
    float n_samples = 0.0; 
    for(float phi = 0.0; phi < 2.0 * PI; phi += sample_delta) {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sample_delta) {
            float3 tangent = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sample_vec = tangent.x * right + tangent.y * up + tangent.z * normal;

            float3 sample_color = EnvironmentMap.Sample(CubeSampler, sample_vec).rgb * cos(theta) * sin(theta);
            irradiance += sample_color;
            n_samples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(n_samples));

    return irradiance;
}

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
	int faceWidth, faceHeight, whatever;
	IrradianceMap.GetDimensions(faceWidth, faceHeight, whatever);

	float2 UV = float2(ThreadID.xy) / float2(faceWidth, faceHeight);
	if (UV.x > 1 || UV.y > 1)
		return;
	float3 LocalDirection = normalize(ToLocalDirection(UV, ThreadID.z));
	LocalDirection.y = -LocalDirection.y;

	float4 Color = float4(calculate_irradiance(LocalDirection), 1);
	IrradianceMap[ThreadID] = Color;
}