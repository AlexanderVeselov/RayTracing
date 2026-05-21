/*****************************************************************************
 MIT License

 Copyright(c) 2026 Alexander Veselov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this softwareand associated documentation files(the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions :

 The above copyright noticeand this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *****************************************************************************/

#include "common.hlsli"

RWStructuredBuffer<Ray> g_ShadowRays : register(u0);
RWStructuredBuffer<uint> g_ShadowRayCounter : register(u1);
RWStructuredBuffer<uint> g_ShadowHits : register(u2);
RaytracingAccelerationStructure g_TLAS : register(t3);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint ray_idx = dispatch_thread_id.x;
    if (ray_idx >= g_ShadowRayCounter[0])
    {
        return;
    }

    Ray ray = g_ShadowRays[ray_idx];

    RayDesc ray_desc;
    ray_desc.Origin = ray.origin;
    ray_desc.TMin = ray.t_min;
    ray_desc.Direction = ray.direction;
    ray_desc.TMax = ray.t_max;

    RayQuery<RAY_FLAG_NONE> query;
    query.TraceRayInline(
        g_TLAS, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff, ray_desc);
    while (query.Proceed())
    {
    }

    g_ShadowHits[ray_idx] = query.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0u : INVALID_ID;
}
