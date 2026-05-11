#include "common.hlsli"

RWStructuredBuffer<uint> g_RayCounter : register(u4);
RWStructuredBuffer<uint> g_PixelIndices : register(u5);
RWStructuredBuffer<Hit> g_Hits : register(u7);
RWStructuredBuffer<float4> g_Throughputs : register(u12);
RWStructuredBuffer<float4> g_Radiance : register(u13);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint ray_idx = dispatch_thread_id.x;
    if (ray_idx >= g_RayCounter[0])
    {
        return;
    }

    Hit hit = g_Hits[ray_idx];
    if (hit.primitive_id != INVALID_ID)
    {
        return;
    }

    uint pixel_idx = g_PixelIndices[ray_idx];
    float3 throughput = g_Throughputs[pixel_idx].xyz;
    g_Radiance[pixel_idx].xyz += throughput * float3(0.02f, 0.02f, 0.025f);
}
