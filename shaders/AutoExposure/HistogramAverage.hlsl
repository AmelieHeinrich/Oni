/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-11 13:03:17
 */

// Special thanks to https://alextardif.com/HistogramLuminance.html

#include "shaders/Common/Math.hlsl"

#define NUM_HISTOGRAM_BINS 256
#define HISTOGRAM_AVERAGE_THREADS_PER_DIMENSION 16

struct LuminanceAverageData
{
    uint LuminanceHistogram;
    uint LuminanceOutput;

    uint pixelCount;
    float minLogLuminance;
    float logLuminanceRange;
    float timeDelta;
    float tau;
};

ConstantBuffer<LuminanceAverageData> Data : register(b0);

groupshared float HistogramShared[NUM_HISTOGRAM_BINS];

[numthreads(HISTOGRAM_AVERAGE_THREADS_PER_DIMENSION, HISTOGRAM_AVERAGE_THREADS_PER_DIMENSION, 1)]
void Main(uint groupIndex : SV_GroupIndex)
{
    RWByteAddressBuffer LuminanceHistogram = ResourceDescriptorHeap[Data.LuminanceHistogram];
    RWTexture2D<float> LuminanceOutput = ResourceDescriptorHeap[Data.LuminanceOutput];

    float countForThisBin = (float)LuminanceHistogram.Load(groupIndex * 4);
    HistogramShared[groupIndex] = countForThisBin * (float)groupIndex;
    
    GroupMemoryBarrierWithGroupSync();
    
    [unroll]
    for(uint histogramSampleIndex = (NUM_HISTOGRAM_BINS >> 1); histogramSampleIndex > 0; histogramSampleIndex >>= 1) {
        if (groupIndex < histogramSampleIndex) {
            HistogramShared[groupIndex] += HistogramShared[groupIndex + histogramSampleIndex];
        }

        GroupMemoryBarrierWithGroupSync();
    }
    
    if (groupIndex == 0) {
        float weightedLogAverage = (HistogramShared[0].x / max((float)Data.pixelCount - countForThisBin, 1.0)) - 1.0;
        float weightedAverageLuminance = exp2(((weightedLogAverage / 254.0) * Data.logLuminanceRange) + Data.minLogLuminance);
        float luminanceLastFrame = LuminanceOutput[uint2(0, 0)];
        float adaptedLuminance = luminanceLastFrame + (weightedAverageLuminance - luminanceLastFrame) * (1 - exp(-Data.timeDelta * Data.tau));
        LuminanceOutput[uint2(0, 0)] = adaptedLuminance;
    }
}
