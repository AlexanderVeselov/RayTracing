/*****************************************************************************
 MIT License

 Copyright(c) 2023 Alexander Veselov

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

#include "src/kernels/common/shared_structures.h"
#include "src/kernels/common/constants.h"

bool RayTriangle(Ray ray, const __global RTTriangle* __restrict tri, uint* prim_id, float* out_t)
{
    float3 e1 = tri->position2 - tri->position1;
    float3 e2 = tri->position3 - tri->position1;

    float3 pvec = cross(ray.direction.xyz, e2);
    float det = dot(e1, pvec);
    if (fabs(det) < 1e-8f) return false;

    float inv_det = 1.0f / det;

    float3 tvec = ray.origin.xyz - tri->position1;
    float  u = dot(tvec, pvec) * inv_det;
    if ((u < 0.0f) | (u > 1.0f)) return false;

    float3 qvec = cross(tvec, e1);
    float  v = dot(ray.direction.xyz, qvec) * inv_det;
    if ((v < 0.0f) | ((u + v) > 1.0f)) return false;

    float t = dot(e2, qvec) * inv_det;
    float t_min = ray.origin.w;
    float t_max = ray.direction.w;
    if ((t < t_min) | (t > t_max)) return false;

    *prim_id = tri->prim_id;
    *out_t = t;
    return true;
}

bool RayBounds(float3 bmin, float3 bmax, float3 ray_origin, float3 ray_inv_dir, float tmin_in, float tmax_in)
{
    float3 t0 = (bmin - ray_origin) * ray_inv_dir;
    float3 t1 = (bmax - ray_origin) * ray_inv_dir;

    float3 tmin3 = fmin(t0, t1);
    float3 tmax3 = fmax(t0, t1);

    float tmin = fmax(fmax(tmin3.x, tmin3.y), fmax(tmin3.z, tmin_in));
    float tmax = fmin(fmin(tmax3.x, tmax3.y), fmin(tmax3.z, tmax_in));
    return tmax >= tmin;
}

__kernel void TraceBvh
(
    // Input
    __global Ray* rays,
    __global uint* ray_counter,
    __global RTTriangle* __restrict triangles,
    __global LinearBVHNode* __restrict nodes,
    // Output
#ifdef SHADOW_RAYS
    __global uint* shadow_hits
#else
    __global Hit* hits
#endif
)
{
    uint ray_idx = get_global_id(0);
    ///@TODO: use indirect dispatch
    uint num_rays = ray_counter[0];

    if (ray_idx >= num_rays)
    {
        return;
    }

    Ray ray = rays[ray_idx];
    // TODO: fix it
    float3 ray_inv_dir = (float3)(1.0f, 1.0f, 1.0f) / ray.direction.xyz;
    int ray_sign[3];
    ray_sign[0] = ray_inv_dir.x < 0;
    ray_sign[1] = ray_inv_dir.y < 0;
    ray_sign[2] = ray_inv_dir.z < 0;

#ifdef SHADOW_RAYS
    uint shadow_hit = INVALID_ID;
#endif

    Hit hit;
    hit.primitive_id = INVALID_ID;

    float t;
    // Follow ray through BVH nodes to find primitive intersections
    int toVisitOffset = 0;
    int currentNodeIndex = 0;
    int nodesToVisit[64];
    unsigned int iter_count = 0;

    while (true)
    {
        LinearBVHNode node = nodes[currentNodeIndex];

        if (RayBounds(node.bmin, node.bmax, ray.origin.xyz, ray_inv_dir, ray.origin.w, ray.direction.w))
        {
            int num_primitives = node.num_primitives_axis >> 16;
            // Leaf node
            if (num_primitives > 0)
            {
                // Intersect ray with primitives in leaf BVH node
                for (int i = 0; i < num_primitives; ++i)
                {
                    if (RayTriangle(ray, &triangles[node.offset + i], &hit.primitive_id, &t))
                    {
                        // Set ray t_max
                        ray.direction.w = t;
                        hit.padding = iter_count;

#ifdef SHADOW_RAYS
                        shadow_hit = 0;
                        goto endtrace;
#endif
                    }
                }

                if (toVisitOffset == 0)
                {
                    break;
                }

                currentNodeIndex = nodesToVisit[--toVisitOffset];
            }
            else
            {
                // Put far BVH node on _nodesToVisit_ stack, advance to near node
                if (ray_sign[node.num_primitives_axis & 0xFFFF])
                {
                    nodesToVisit[toVisitOffset++] = currentNodeIndex + 1;
                    currentNodeIndex = node.offset;
                }
                else
                {
                    nodesToVisit[toVisitOffset++] = node.offset;
                    currentNodeIndex = currentNodeIndex + 1;
                }
            }
        }
        else
        {
            if (toVisitOffset == 0)
            {
                break;
            }

            currentNodeIndex = nodesToVisit[--toVisitOffset];
        }

        ++iter_count;
    }

endtrace:
    // Write the result to the output buffer
#ifdef SHADOW_RAYS
    shadow_hits[ray_idx] = shadow_hit;
#else
    hits[ray_idx] = hit;
#endif
}
