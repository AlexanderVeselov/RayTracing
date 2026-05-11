#include "common.hlsli"

RWStructuredBuffer<uint> g_ShadowRayCounter : register(u9);
RWStructuredBuffer<uint> g_ShadowPixelIndices : register(u10);
RWStructuredBuffer<uint> g_ShadowHits : register(u11);
RWStructuredBuffer<float4> g_Radiance : register(u13);
RWStructuredBuffer<float4> g_DirectLightSamples : register(u14);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint ray_idx = dispatch_thread_id.x;
    if (ray_idx >= g_ShadowRayCounter[0])
    {
        return;
    }

    if (g_ShadowHits[ray_idx] == INVALID_ID)
    {
        uint pixel_idx = g_ShadowPixelIndices[ray_idx];
        g_Radiance[pixel_idx].xyz += g_DirectLightSamples[ray_idx].xyz;
    }
}
