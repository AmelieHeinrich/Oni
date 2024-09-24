/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-11 12:20:34
 */

// Special thanks to https://alextardif.com/HistogramLuminance.html

#include "shaders/Common/Colors.hlsl"

#define NUM_HISTOGRAM_BINS 256
#define HISTOGRAM_THREADS_PER_DIMENSION 16

struct LuminanceHistogramData
{
    uint HDRTexture;
    uint LuminanceHistogram;

    uint inputWidth;
    uint inputHeight;
    float minLogLuminance;
    float oneOverLogLuminanceRange;
};

ConstantBuffer<LuminanceHistogramData> Parameters : register(b0);

groupshared uint HistogramShared[NUM_HISTOGRAM_BINS];

uint HDRToHistogramBin(float3 hdrColor)
{
    float luminance = Luminance(hdrColor);
    
    if (luminance < Epsilon) {
        return 0;
    }
    
    float logLuminance = saturate((log2(luminance) - Parameters.minLogLuminance) * Parameters.oneOverLogLuminanceRange);
    return (uint)(logLuminance * 254.0 + 1.0);
}

[numthreads(HISTOGRAM_THREADS_PER_DIMENSION, HISTOGRAM_THREADS_PER_DIMENSION, 1)]
void Main(uint groupIndex : SV_GroupIndex, uint3 threadId : SV_DispatchThreadID)
{
    Texture2D HDRTexture = ResourceDescriptorHeap[Parameters.HDRTexture];
    RWByteAddressBuffer LuminanceHistogram = ResourceDescriptorHeap[Parameters.LuminanceHistogram];

    HistogramShared[groupIndex] = 0;
    
    GroupMemoryBarrierWithGroupSync();
    
    if(threadId.x < Parameters.inputWidth && threadId.y < Parameters.inputHeight)
    {
        float3 hdrColor = HDRTexture.Load(int3(threadId.xy, 0)).rgb;
        uint binIndex = HDRToHistogramBin(hdrColor);
        InterlockedAdd(HistogramShared[binIndex], 1);
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    LuminanceHistogram.InterlockedAdd(groupIndex * 4, HistogramShared[groupIndex]);
}
