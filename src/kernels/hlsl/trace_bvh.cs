#include "trace_common.hlsli"

RWStructuredBuffer<Ray> g_Rays : register(u0);
RWStructuredBuffer<uint> g_RayCounter : register(u1);
RWStructuredBuffer<Hit> g_Hits : register(u2);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint ray_idx = dispatch_thread_id.x;
    if (ray_idx >= g_RayCounter[0])
    {
        return;
    }

    uint node_count;
    uint stride;
    g_Nodes.GetDimensions(node_count, stride);
    Ray ray = g_Rays[ray_idx];
    g_Hits[ray_idx] = TraceBVH(
        ray.origin.xyz, ray.direction.xyz, ray.origin.w, ray.direction.w, node_count, false);
}
