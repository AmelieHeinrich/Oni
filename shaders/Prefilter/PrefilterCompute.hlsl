/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 00:39:00
 */

static const float PI = 3.141592;
static const float TwoPI = 2 * PI;
static const float Epsilon = 0.00001;

static const uint NumSamples = 1;
static const float InvNumSamples = 1.0 / float(NumSamples);

struct PrefilterMapSettings
{
	float4 Roughness;
};

TextureCube EnvironmentMap : register(t0);
RWTexture2DArray<float4> PrefilterMap : register(u1);
SamplerState CubeSampler : register(s2);
ConstantBuffer<PrefilterMapSettings> PrefilterSettings : register(b3);

float2 sampleHammersley(uint i, uint N)
{
	float fbits;
    uint bits = i;
    
    bits  = (bits << 16u) | (bits >> 16u);
    bits  = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits  = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits  = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits  = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    fbits = float(bits) * 2.3283064365386963e-10;

	return float2(float(i) / float(N), fbits);
}

float3 sampleGGX(float2 Xi, float3 N, float roughness)
{
	float alpha = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);
	
	float3 sampleVec = tangent * float3(H.x, H.x, H.x) + bitangent * float3(H.y, H.y, H.y) + N * float3(H.z, H.z, H.z);
	return normalize(sampleVec);
}

float ndfGGX(float3 N, float3 H, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSq = alpha * alpha;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = alphaSq;
	float denom = (NdotH2 * (alphaSq - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float3 getSamplingVector(uint3 ThreadID)
{
	float outputWidth, outputHeight, outputDepth;
	PrefilterMap.GetDimensions(outputWidth, outputHeight, outputDepth);

    float2 st = ThreadID.xy / float2(outputWidth, outputHeight);
    float2 uv = 2.0 * float2(st.x, 1.0 - st.y) - 1.0;

	// Select vector based on cubemap face index.
	float3 ret = float3(1.0f, 1.0f, 1.0f);
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

void computeBasisVectors(const float3 N, out float3 S, out float3 T)
{
	// Branchless select non-degenerate T.
	T = cross(N, float3(0.0, 1.0, 0.0));
	T = lerp(cross(N, float3(1.0, 0.0, 0.0)), T, step(Epsilon, dot(T, T)));

	T = normalize(T);
	S = normalize(cross(N, T));
}

float3 tangentToWorld(const float3 v, const float3 N, const float3 S, const float3 T)
{
	return S * v.x + T * v.y + N * v.z;
}

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
	// Make sure we won't write past output when computing higher mipmap levels.
	uint outputWidth, outputHeight, outputDepth;
	PrefilterMap.GetDimensions(outputWidth, outputHeight, outputDepth);
	if(ThreadID.x >= outputWidth || ThreadID.y >= outputHeight) {
		return;
	}
	
	// Get input cubemap dimensions at zero mipmap level.
	float inputWidth, inputHeight, inputLevels;
	EnvironmentMap.GetDimensions(0, inputWidth, inputHeight, inputLevels);

	// Solid angle associated with a single cubemap texel at zero mipmap level.
	// This will come in handy for importance sampling below.
	float wt = 4.0 * PI / (6 * inputWidth * inputHeight);
	
	// Approximation: Assume zero viewing angle (isotropic reflections).
	float3 N = getSamplingVector(ThreadID);
	float3 Lo = N;
	
	float3 S, T;
	computeBasisVectors(N, S, T);

	float3 color = 0;
	float weight = 0;

	for (uint i = 0; i < NumSamples; i++) {
		float2 Xi = sampleHammersley(i, NumSamples);
		float3 H = sampleGGX(Xi, N, PrefilterSettings.Roughness.x);
		float3 L = normalize(2.0 * dot(N, H) * H - N);

		float NdotL = max(dot(N, L), 0.0);
		if (NdotL > 0.0) {
			float D = ndfGGX(N, H, PrefilterSettings.Roughness.x);
			float NdotH = max(dot(N, H), 0.0);
			float HdotV = max(dot(H, N), 0.0);
			float pdf = D * NdotH / (4.0 * HdotV + 0.00001);

			float saTexel = 4.0 * PI / (6.0 * inputWidth.x * inputWidth.x);
			float saSample = 1.0 / (float(NumSamples) * pdf + 0.00001);

			float mipLevel = PrefilterSettings.Roughness.x == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

			color += EnvironmentMap.SampleLevel(CubeSampler, L, mipLevel).rgb * NdotL;
			weight += NdotL;
		}
	}
	color /= weight;

	PrefilterMap[ThreadID] = float4(color, 1.0);
}