/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-01 00:40:47
 */

struct FragmentIn
{
	float4 Position : SV_POSITION;
	float3 LocalPosition : COLOR0;
};

TextureCube EnvironmentMap : register(t0);
SamplerState CubeSampler : register(s0);

float4 Main(FragmentIn input) : SV_Target
{
	float3 env_vector = normalize(input.LocalPosition);
	return EnvironmentMap.SampleLevel(CubeSampler, env_vector, 0);
}
