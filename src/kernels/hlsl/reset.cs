#include "common.hlsli"

RWStructuredBuffer<float4> g_Radiance : register(u13);
RWStructuredBuffer<uint> g_SampleCounter : register(u15);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint idx = dispatch_thread_id.x;
    uint radiance_count;
    uint stride;
    g_Radiance.GetDimensions(radiance_count, stride);

    if (idx == 0)
    {
        g_SampleCounter[0] = 0;
    }

    if (idx < radiance_count)
    {
        g_Radiance[idx] = 0.0f.xxxx;
    }
}
