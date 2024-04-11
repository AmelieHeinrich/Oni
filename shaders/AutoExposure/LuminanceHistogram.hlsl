/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-04-11 12:20:34
 */

// Special thanks to https://alextardif.com/HistogramLuminance.html

#define NUM_HISTOGRAM_BINS 256
#define HISTOGRAM_THREADS_PER_DIMENSION 16
#define EPSILON 0.00001

struct LuminanceHistogramData
{
    uint inputWidth;
    uint inputHeight;
    float minLogLuminance;
    float oneOverLogLuminanceRange;
};

Texture2D HDRTexture : register(t0);
RWByteAddressBuffer LuminanceHistogram : register(u1);
ConstantBuffer<LuminanceHistogramData> Parameters : register(b2);

groupshared uint HistogramShared[NUM_HISTOGRAM_BINS];

float GetLuminance(float3 color)
{
    return dot(color, float3(0.2127f, 0.7152f, 0.0722f));
}

uint HDRToHistogramBin(float3 hdrColor)
{
    float luminance = GetLuminance(hdrColor);
    
    if (luminance < EPSILON) {
        return 0;
    }
    
    float logLuminance = saturate((log2(luminance) - Parameters.minLogLuminance) * Parameters.oneOverLogLuminanceRange);
    return (uint)(logLuminance * 254.0 + 1.0);
}

[numthreads(HISTOGRAM_THREADS_PER_DIMENSION, HISTOGRAM_THREADS_PER_DIMENSION, 1)]
void Main(uint groupIndex : SV_GroupIndex, uint3 threadId : SV_DispatchThreadID)
{
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
