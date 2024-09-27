//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-25 19:58:57
//

#include "shaders/Common/Compute.hlsl"
#include "shaders/Common/Math.hlsl"

struct Constants
{
    uint Velocity;
    uint OutputHDR;
    uint NearestSampler;
};

ConstantBuffer<Constants> Settings : register(b0);

static const int ARROW_V_STYLE = 1;
static const int ARROW_LINE_STYLE = 2;
static const int ARROW_STYLE = ARROW_LINE_STYLE;
static const float ARROW_TILE_SIZE = 64.0;
static const float ARROW_HEAD_ANGLE = 45.0 * PI / 180.0;
static const float ARROW_HEAD_LENGTH = ARROW_TILE_SIZE / 6.0;
static const float ARROW_SHAFT_THICKNESS = 3.0;

float2 ArrowTileCenterCoord(float2 pos)
{
    return (floor(pos / ARROW_TILE_SIZE) + 0.5) * ARROW_TILE_SIZE;
}

float Arrow(float2 p, float2 v)
{
    // Make everything relative to the center, which may be fractional
    p -= ArrowTileCenterCoord(p);
        
    float mag_v = length(v), mag_p = length(p);
    
    if (mag_v > 0.0) {
        // Non-zero velocity case
        float2 dir_p = p / mag_p, dir_v = v / mag_v;
        
        // We can't draw arrows larger than the tile radius, so clamp magnitude.
        // Enforce a minimum length to help see direction
        mag_v = clamp(mag_v, 5.0, ARROW_TILE_SIZE / 2.0);

        // Arrow tip location
        v = dir_v * mag_v;
        
        // Define a 2D implicit surface so that the arrow is antialiased.
        // In each line, the left expression defines a shape and the right controls
        // how quickly it fades in or out.

        float dist;		
        if (ARROW_STYLE == ARROW_LINE_STYLE) {
            // Signed distance from a line segment based on https://www.shadertoy.com/view/ls2GWG by 
            // Matthias Reitinger, @mreitinger
            
            // Line arrow style
            dist = 
                max(
                    // Shaft
                    ARROW_SHAFT_THICKNESS / 4.0 - 
                        max(abs(dot(p, float2(dir_v.y, -dir_v.x))), // Width
                            abs(dot(p, dir_v)) - mag_v + ARROW_HEAD_LENGTH / 2.0), // Length
                        
                        // Arrow head
                     min(0.0, dot(v - p, dir_v) - cos(ARROW_HEAD_ANGLE / 2.0) * length(v - p)) * 2.0 + // Front sides
                     min(0.0, dot(p, dir_v) + ARROW_HEAD_LENGTH - mag_v)); // Back
        } else {
            // V arrow style
            dist = min(0.0, mag_v - mag_p) * 2.0 + // length
                   min(0.0, dot(normalize(v - p), dir_v) - cos(ARROW_HEAD_ANGLE / 2.0)) * 2.0 * length(v - p) + // head sides
                   min(0.0, dot(p, dir_v) + 1.0) + // head back
                   min(0.0, cos(ARROW_HEAD_ANGLE / 2.0) - dot(normalize(v * 0.33 - p), dir_v)) * mag_v * 0.8; // cutout
        }
        
        return clamp(1.0 + dist, 0.0, 1.0);
    } else {
        // Center of the pixel is always on the arrow
        return max(0.0, 1.2 - mag_p);
    }
}

float2 Field(uint2 ThreadID)
{
    Texture2D<float2> Velocity = ResourceDescriptorHeap[Settings.Velocity];
    SamplerState NearestSampler = SamplerDescriptorHeap[Settings.NearestSampler];
    
    int width, height;
    Velocity.GetDimensions(width, height);

    float2 texelSize = 1.0 / float2(width, height);
    float2 UVs = TexelToUV(ThreadID, texelSize);

    return Velocity.Sample(NearestSampler, UVs);
}

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    RWTexture2D<float4> OutputHDR = ResourceDescriptorHeap[Settings.OutputHDR];

    float4 FragColor = (1.0 - Arrow(ThreadID.xy, Field(ArrowTileCenterCoord(ThreadID.xy)) * ARROW_TILE_SIZE * 0.4)) * float4((Field(ThreadID.xy) * float2(1000.0, 1000.0)) * 0.5 + 0.5, 0.5, 1.0);
    OutputHDR[ThreadID.xy] = FragColor;
}
