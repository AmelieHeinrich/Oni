//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-28 22:10:19
//

#include "shaders/Common/Math.hlsl"

struct Constants
{
    uint Texture;
    float Amount;
    float FrameTime;
};

ConstantBuffer<Constants> Settings : register(b0);

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    RWTexture2D<float4> Output = ResourceDescriptorHeap[Settings.Texture];

    float amount = Settings.Amount;
    float randomIntensity = frac(10000 * sin((ThreadID.x + ThreadID.y * Settings.FrameTime) * PI));
    amount *= randomIntensity;

    Output[ThreadID.xy] += amount;
}
