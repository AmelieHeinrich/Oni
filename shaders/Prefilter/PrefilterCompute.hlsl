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
RWTexture2DArray<half4> PrefilterMap : register(u1);
SamplerState CubeSampler : register(s2);
ConstantBuffer<PrefilterMapSettings> PrefilterSettings : register(b3);

float3 CubeToWorld(int3 cubeCoord, float2 cubeSize)
{
	float2 texCoord = float2(cubeCoord.xy) / cubeSize;
    texCoord = texCoord  * 2.0 - 1.0; // Swap to -1 -> +1
    switch(cubeCoord.z)
    {
        case 0: return float3(1.0, -texCoord.yx); // CUBE_MAP_POS_X
        case 1: return float3(-1.0, -texCoord.y, texCoord.x); // CUBE_MAP_NEG_X
        case 2: return float3(texCoord.x, 1.0, texCoord.y); // CUBE_MAP_POS_Y
        case 3: return float3(texCoord.x, -1.0, -texCoord.y); // CUBE_MAP_NEG_Y
        case 4: return float3(texCoord.x, -texCoord.y, 1.0); // CUBE_MAP_POS_Z
        case 5: return float3(-texCoord.xy, -1.0); // CUBE_MAP_NEG_Z
    }
    return float3(0.0, 0.0, 0.0);
}

int3 TexToCube(float3 texCoord, float2 cubeSize)
{
	float3 abst = abs(texCoord);
    texCoord /= max(max(abst.x, abst.y), abst.z);
    
    float cubeFace;
    float2 uvCoord;
    if (abst.x > abst.y && abst.x > abst.z)
    {
        // X major.
        float negx = step(texCoord.x, 0.0);
        uvCoord = lerp(-texCoord.zy, float2(texCoord.z, -texCoord.y), negx);
        cubeFace = negx;
    }
    else if (abst.y > abst.z)
    {
        // Y major.
        float negy = step(texCoord.y, 0.0);
        uvCoord = lerp(texCoord.xz, float2(texCoord.x, -texCoord.z), negy);
        cubeFace = 2.0 + negy;
    }
    else
    {
        // Z major.
        float negz = step(texCoord.z, 0.0);
        uvCoord = lerp(float2(texCoord.x, -texCoord.y), -texCoord.xy, negz);
        cubeFace = 4.0 + negz;
    }
    uvCoord = (uvCoord + 1.0) * 0.5; // 0..1
    uvCoord = uvCoord * cubeSize;
    uvCoord = clamp(uvCoord, float2(0.0, 0.0), cubeSize - float2(1.0, 1.0));
    
    return int3(int2(uvCoord), int(cubeFace));
}

float SSBGeometry(float3 N, float3 H, float roughness)
{
	float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom       = PI * denom * denom;
    
    return nom / denom;
}

float2 Hammersley(uint i, uint N)
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

float3 SSBImportance(float2 Xi, float3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates - halfway vector
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space H vector to world-space sample vector
	float3 up        = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent   = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);

	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

[numthreads(32, 32, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
	const uint SAMPLE_COUNT = 1;
	
	float enviWidth, enviHeight;
	EnvironmentMap.GetDimensions(enviWidth, enviHeight);
	float2 enviSize = float2(enviWidth, enviHeight);

	float mipWidth, mipHeight, whatever;
	PrefilterMap.GetDimensions(mipWidth, mipHeight, whatever);
	float2 mipSize = float2(mipWidth, mipHeight);	

    int3 cubeCoord = int3(ThreadID.xyz);
    
    float3 worldPos = CubeToWorld(cubeCoord, mipSize);
    float3 N = normalize(worldPos);
    
    float3 prefilteredColor = float3(0.0, 0.0, 0.0);
    float totalWeight = 0.0;
    
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = SSBImportance(Xi, N, PrefilterSettings.Roughness.x);
        float3 L  = normalize(2.0 * dot(N, H) * H - N);
    
        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            float D = SSBGeometry(N, H, PrefilterSettings.Roughness.x);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, N), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV + 0.0001);
    
            float saTexel  = 4.0 * PI / (6.0 * enviSize.x * enviSize.x);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
    
            float mipLevel = PrefilterSettings.Roughness.x == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);
    
            prefilteredColor += EnvironmentMap.SampleLevel(CubeSampler, L, mipLevel).xyz * NdotL;
            totalWeight += NdotL;
        }
    }
    
    prefilteredColor = prefilteredColor / totalWeight;
    
	PrefilterMap[cubeCoord] = float4(prefilteredColor, 1.0); 
}