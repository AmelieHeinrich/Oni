//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-20 18:55:41
//

float2 TexelToUV(uint2 texel, float2 texelSize)
{
	return ((float2)texel + 0.5f) * texelSize;
}
