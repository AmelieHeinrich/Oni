/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 00:39:25
 */

#include "shaders/Common/Math.hlsl"

struct Constants
{
    uint EnvironmentMap;
    uint IrradianceMap;
    uint CubeSampler;
    uint _Pad0;
};

ConstantBuffer<Constants> Settings : register(b0);

static const uint NumSamples = 64 * 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

float2 SampleHammersley(uint i)
{
	return float2(i * InvNumSamples, radicalInverse_VdC(i));
}

float3 SampleHemisphere(float u1, float u2)
{
	const float u1p = sqrt(max(0.0, 1.0 - u1*u1));
	return float3(cos(TwoPI * u2) * u1p, sin(TwoPI * u2) * u1p, u1);
}

float3 GetSamplingVector(uint3 ThreadID)
{
    RWTexture2DArray<half4> IrradianceMap = ResourceDescriptorHeap[Settings.IrradianceMap];

	float outputWidth, outputHeight, outputDepth;
	IrradianceMap.GetDimensions(outputWidth, outputHeight, outputDepth);

    float2 st = ThreadID.xy/float2(outputWidth, outputHeight);
    float2 uv = 2.0 * float2(st.x, 1.0-st.y) - 1.0;

	// Select vector based on cubemap face index.
	float3 ret;
	switch(ThreadID.z)
	{
	    case 0: ret = float3(1.0,  uv.y, -uv.x); break;
	    case 1: ret = float3(-1.0, uv.y,  uv.x); break;
	    case 2: ret = float3(uv.x, 1.0, -uv.y); break;
	    case 3: ret = float3(uv.x, -1.0, uv.y); break;
	    case 4: ret = float3(uv.x, uv.y, 1.0); break;
	    case 5: ret = float3(-uv.x, uv.y, -1.0); break;
	}
    return normalize(ret);
}

void ComputeBasisVectors(const float3 N, out float3 S, out float3 T)
{
	// Branchless select non-degenerate T.
	T = cross(N, float3(0.0, 1.0, 0.0));
	T = lerp(cross(N, float3(1.0, 0.0, 0.0)), T, step(Epsilon, dot(T, T)));

	T = normalize(T);
	S = normalize(cross(N, T));
}

float3 TangentToWorld(const float3 v, const float3 N, const float3 S, const float3 T)
{
	return S * v.x + T * v.y + N * v.z;
}

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    TextureCube EnvironmentMap = ResourceDescriptorHeap[Settings.EnvironmentMap];
    RWTexture2DArray<half4> IrradianceMap = ResourceDescriptorHeap[Settings.IrradianceMap];
    SamplerState CubeSampler = SamplerDescriptorHeap[Settings.CubeSampler];

    float3 N = GetSamplingVector(ThreadID);

    float3 S, T;
	ComputeBasisVectors(N, S, T);

    // Monte Carlo integration of hemispherical irradiance.
	// As a small optimization this also includes Lambertian BRDF assuming perfectly white surface (albedo of 1.0)
	// so we don't need to normalize in PBR fragment shader (so technically it encodes exitant radiance rather than irradiance).
	float3 irradiance = 0.0;
	for(uint i=0; i<NumSamples; ++i) {
		float2 u  = SampleHammersley(i);
		float3 Li = TangentToWorld(SampleHemisphere(u.x, u.y), N, S, T);
		float cosTheta = max(0.0, dot(Li, N));

		// PIs here cancel out because of division by pdf.
		irradiance += 2.0 * EnvironmentMap.SampleLevel(CubeSampler, Li, 0).rgb * cosTheta;
	}
	irradiance /= float(NumSamples);

	IrradianceMap[ThreadID] = float4(irradiance, 1.0);
}