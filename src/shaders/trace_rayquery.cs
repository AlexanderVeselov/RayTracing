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

RWStructuredBuffer<Ray> g_Rays : register(u0);
RWStructuredBuffer<uint> g_RayCounter : register(u1);
RWStructuredBuffer<Hit> g_Hits : register(u2);
RaytracingAccelerationStructure g_TLAS : register(t3);

Hit TraceRayQuery(Ray ray)
{
    Hit hit;
    hit.bc = 0.0f.xx;
    hit.primitive_id = INVALID_ID;
    hit.instance_id = INVALID_ID;
    hit.t = ray.t_max;
    hit.padding1 = 0u;
    hit.padding2 = 0u;
    hit.padding3 = 0u;

    RayDesc ray_desc;
    ray_desc.Origin = ray.origin;
    ray_desc.TMin = ray.t_min;
    ray_desc.Direction = ray.direction;
    ray_desc.TMax = ray.t_max;

    RayQuery<RAY_FLAG_NONE> query;
    query.TraceRayInline(g_TLAS, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, ray_desc);
    while (query.Proceed())
    {
    }

    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        hit.bc = query.CommittedTriangleBarycentrics();
        hit.primitive_id = query.CommittedPrimitiveIndex();
        hit.instance_id = query.CommittedInstanceID();
        hit.t = query.CommittedRayT();
    }

    return hit;
}

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint ray_idx = dispatch_thread_id.x;
    if (ray_idx >= g_RayCounter[0])
    {
        return;
    }

    g_Hits[ray_idx] = TraceRayQuery(g_Rays[ray_idx]);
}
