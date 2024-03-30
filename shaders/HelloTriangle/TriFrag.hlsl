/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:53:11
 */

struct FragmentIn
{
    float4 Position : SV_POSITION;
    float2 TexCoords : TEXCOORD; 
};

Texture2D Texture : register(t1);
SamplerState Sampler : register(s2);

float4 Main(FragmentIn Input) : SV_TARGET
{
    return Texture.Sample(Sampler, Input.TexCoords);
}