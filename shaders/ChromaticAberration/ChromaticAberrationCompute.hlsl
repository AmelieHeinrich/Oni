//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-24 23:21:59
//

struct Constants
{
    uint Input;
    int3 Offsets;
};

ConstantBuffer<Constants> Settings : register(b0);

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    RWTexture2D<float4> Input = ResourceDescriptorHeap[Settings.Input];

    float R = Input[ThreadID.xy + uint2(Settings.Offsets.x, Settings.Offsets.x)].r;
    float G = Input[ThreadID.xy + uint2(Settings.Offsets.y, Settings.Offsets.y)].g;
    float B = Input[ThreadID.xy + uint2(Settings.Offsets.z, Settings.Offsets.z)].b;

    Input[ThreadID.xy] = float4(R, G, B, 1.0);
}
