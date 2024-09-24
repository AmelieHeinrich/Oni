/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-06 16:10:44
 */

struct Parameters
{
	uint SrcTexture;
	uint DstTexture;
	uint BilinearClamp;
	uint _Pad0;
	float4 MipSize;
};

ConstantBuffer<Parameters> Mips : register(b0);

[numthreads(8, 8, 1)]
void Main(uint3 DTid : SV_DispatchThreadID)
{
	// Load bindings
	Texture2D<float4> SrcTexture = ResourceDescriptorHeap[Mips.SrcTexture];
	RWTexture2D<float4> DstTexture = ResourceDescriptorHeap[Mips.DstTexture];
	SamplerState BilinearClamp = SamplerDescriptorHeap[Mips.BilinearClamp];

	//DTid is the thread ID * the values from numthreads above and in this case correspond to the pixels location in number of pixels.
	//As a result texcoords (in 0-1 range) will point at the center between the 4 pixels used for the mipmap.
	float2 texcoords = (1.0f / Mips.MipSize.xy) * (DTid.xy + 0.5);

	//The samplers linear interpolation will mix the four pixel values to the new pixels color
	float4 color = SrcTexture.SampleLevel(BilinearClamp, texcoords, 0);

	//Write the final color into the destination texture.
	DstTexture[DTid.xy] = color;
}
