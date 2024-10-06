//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-05 12:25:30
//

struct Constants
{
    uint Output;
    uint BlurSize;
    uint2 Pad;
};

ConstantBuffer<Constants> Settings : register(b0);

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    RWTexture2D<float> Output = ResourceDescriptorHeap[Settings.Output];
    
    float result = 0.0;
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            float2 offset = ThreadID.xy + uint2(x, y);
            result += Output[offset];
        }
    }
    Output[ThreadID.xy] = result / (5.0 * 5.0);
}
