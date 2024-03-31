/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-30 14:53:11
 */

struct FragmentIn
{
    float4 Position : SV_POSITION;
    float2 TexCoords : TEXCOORD; 
};

Texture2D Texture : register(t2);
SamplerState Sampler : register(s3);

float4 Main(FragmentIn Input) : SV_TARGET
{
    float4 color = Texture.Sample(Sampler, Input.TexCoords);
    if (color.a < 0.25)
        discard;
    return color;
}