#include "common.hlsli"
#include "frame_data.hlsli"

RWStructuredBuffer<float4> g_Radiance : register(u13);
RWStructuredBuffer<uint> g_SampleCounter : register(u15);
StructuredBuffer<float4> g_PrevRadiance : register(t27);
RWStructuredBuffer<float> g_Depth : register(u17);
StructuredBuffer<float> g_PrevDepth : register(t28);
RWStructuredBuffer<float4> g_MotionVectors : register(u26);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint pixel_idx = dispatch_thread_id.x;
    uint width = g_RenderSize.x;
    uint height = g_RenderSize.y;
    uint num_pixels = width * height;
    if (pixel_idx >= num_pixels)
    {
        return;
    }
    if (g_SampleCounter[0] <= 1u)
    {
        return;
    }

    uint x = pixel_idx % width;
    uint y = pixel_idx / width;
    float depth_value = g_Depth[pixel_idx];
    if (depth_value == MAX_RENDER_DIST)
    {
        return;
    }

    float2 motion = g_MotionVectors[pixel_idx].xy;
    float2 prev_uv = (float2(float(x) + 0.5f, float(y) + 0.5f) / float2(width, height)) - motion;
    int prev_x = int(prev_uv.x * float(width));
    int prev_y = int(prev_uv.y * float(height));
    if (prev_x < 0 || prev_x >= int(width) || prev_y < 0 || prev_y >= int(height))
    {
        return;
    }

    uint prev_idx = uint(prev_y) * width + uint(prev_x);
    float prev_depth_value = g_PrevDepth[prev_idx];
    if (abs(depth_value - prev_depth_value) / max(depth_value, EPS) > 0.1f)
    {
        return;
    }

    float3 current_radiance = g_Radiance[pixel_idx].xyz;
    float3 prev_radiance = g_PrevRadiance[prev_idx].xyz;
    g_Radiance[pixel_idx].xyz = lerp(current_radiance, prev_radiance, 0.9f);
}
