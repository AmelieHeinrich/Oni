/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 00:39:00
 */

#include "shaders/Common/Math.hlsl"

static const uint NumSamples = 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

struct PrefilterMapSettings
{
    uint EnvironmentMap;
    uint PrefilterMap;
    uint CubeSampler;
    float Roughness;
};

ConstantBuffer<PrefilterMapSettings> PrefilterSettings : register(b0);

float2 SampleHammersley(uint i)
{
	return float2(i * InvNumSamples, radicalInverse_VdC(i));
}

float3 SampleGGX(float u1, float u2, float roughness)
{
	float alpha = roughness * roughness;

	float cosTheta = sqrt((1.0 - u2) / (1.0 + (alpha*alpha - 1.0) * u2));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta); // Trig. identity
	float phi = TwoPI * u1;

	// Convert to Cartesian upon return.
	return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float NdfGGX(float cosLh, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Calculate normalized sampling direction vector based on current fragment coordinates.
// This is essentially "inverse-sampling": we reconstruct what the sampling vector would be if we wanted it to "hit"
// this particular fragment in a cubemap.
float3 GetSamplingVector(uint3 ThreadID)
{
    RWTexture2DArray<half4> PrefilterMap = ResourceDescriptorHeap[PrefilterSettings.PrefilterMap];

	float outputWidth, outputHeight, outputDepth;
	PrefilterMap.GetDimensions(outputWidth, outputHeight, outputDepth);

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

// Compute orthonormal basis for converting from tanget/shading space to world space.
void ComputeBasisVectors(const float3 N, out float3 S, out float3 T)
{
	// Branchless select non-degenerate T.
	T = cross(N, float3(0.0, 1.0, 0.0));
	T = lerp(cross(N, float3(1.0, 0.0, 0.0)), T, step(Epsilon, dot(T, T)));

	T = normalize(T);
	S = normalize(cross(N, T));
}

// Convert point from tangent/shading space to world space.
float3 TangentToWorld(const float3 v, const float3 N, const float3 S, const float3 T)
{
	return S * v.x + T * v.y + N * v.z;
}

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    TextureCube EnvironmentMap = ResourceDescriptorHeap[PrefilterSettings.EnvironmentMap];
    RWTexture2DArray<half4> PrefilterMap = ResourceDescriptorHeap[PrefilterSettings.PrefilterMap];
    SamplerState CubeSampler = ResourceDescriptorHeap[PrefilterSettings.CubeSampler];
	
    uint outputWidth, outputHeight, outputDepth;
	PrefilterMap.GetDimensions(outputWidth, outputHeight, outputDepth);
	if(ThreadID.x >= outputWidth || ThreadID.y >= outputHeight) {
		return;
	}

    float inputWidth, inputHeight, inputLevels;
	EnvironmentMap.GetDimensions(0, inputWidth, inputHeight, inputLevels);
	
    // Solid angle associated with a single cubemap texel at zero mipmap level.
	// This will come in handy for importance sampling below.
	float wt = 4.0 * PI / (6 * inputWidth * inputHeight);
	
	// Approximation: Assume zero viewing angle (isotropic reflections).
	float3 N = GetSamplingVector(ThreadID);
	float3 Lo = N;
	
	float3 S, T;
	ComputeBasisVectors(N, S, T);

	float3 color = 0;
	float weight = 0;

	// Convolve environment map using GGX NDF importance sampling.
	// Weight by cosine term since Epic claims it generally improves quality.
	for(uint i = 0; i < NumSamples; ++i) {
		float2 u = SampleHammersley(i);
		float3 Lh = TangentToWorld(SampleGGX(u.x, u.y, PrefilterSettings.Roughness), N, S, T);

		// Compute incident direction (Li) by reflecting viewing direction (Lo) around half-vector (Lh).
		float3 Li = 2.0 * dot(Lo, Lh) * Lh - Lo;

		float cosLi = dot(N, Li);
		if(cosLi > 0.0) {
			// Use Mipmap Filtered Importance Sampling to improve convergence.
			// See: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html, section 20.4

			float cosLh = max(dot(N, Lh), 0.0);

			// GGX normal distribution function (D term) probability density function.
			// Scaling by 1/4 is due to change of density in terms of Lh to Li (and since N=V, rest of the scaling factor cancels out).
			float pdf = NdfGGX(cosLh, PrefilterSettings.Roughness) * 0.25;

			// Solid angle associated with this sample.
			float ws = 1.0 / (NumSamples * pdf);

			// Mip level to sample from.
			float mipLevel = max(0.5 * log2(ws / wt) + 1.0, 0.0);

			color  += EnvironmentMap.SampleLevel(CubeSampler, Li, mipLevel).rgb * cosLi;
			weight += cosLi;
		}
	}
	color /= weight;
    color = pow(color, 2.2);

	PrefilterMap[ThreadID] = float4(color, 1.0); 
}