#include "trace_common.hlsli"

RWStructuredBuffer<Ray> g_ShadowRays : register(u8);
RWStructuredBuffer<uint> g_ShadowRayCounter : register(u9);
RWStructuredBuffer<uint> g_ShadowHits : register(u11);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint ray_idx = dispatch_thread_id.x;
    if (ray_idx >= g_ShadowRayCounter[0])
    {
        return;
    }

    uint node_count;
    uint stride;
    g_Nodes.GetDimensions(node_count, stride);
    Ray ray = g_ShadowRays[ray_idx];
    Hit hit = TraceBVH(
        ray.origin.xyz, ray.direction.xyz, ray.origin.w, ray.direction.w, node_count, true);
    g_ShadowHits[ray_idx] = hit.primitive_id == INVALID_ID ? INVALID_ID : 0u;
}
