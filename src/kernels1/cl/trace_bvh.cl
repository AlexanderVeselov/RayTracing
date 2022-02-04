/*****************************************************************************
 MIT License

 Copyright(c) 2021 Alexander Veselov

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

#include "shared_structures.h"
#include "constants.h"

bool RayTriangle(Ray ray, const __global Triangle* triangle, float2* bc, float* out_t)
{
    float3 e1 = triangle->v2.position - triangle->v1.position;
    float3 e2 = triangle->v3.position - triangle->v1.position;
    // Calculate planes normal vector
    float3 pvec = cross(ray.direction.xyz, e2);
    float det = dot(e1, pvec);

    // Ray is parallel to plane
    if (det < 1e-8f || -det > 1e-8f)
    {
        return false;
    }

    float inv_det = 1.0f / det;
    float3 tvec = ray.origin.xyz - triangle->v1.position;
    float u = dot(tvec, pvec) * inv_det;

    if (u < 0.0f || u > 1.0f)
    {
        return false;
    }

    float3 qvec = cross(tvec, e1);
    float v = dot(ray.direction.xyz, qvec) * inv_det;

    if (v < 0.0f || u + v > 1.0f)
    {
        return false;
    }

    float t = dot(e2, qvec) * inv_det;
    float t_min = ray.origin.w;
    float t_max = ray.direction.w;

    if (t < t_min || t > t_max)
    {
        return false;
    }

    // Intersection is found
    *bc = (float2)(u, v);
    *out_t = t;

    return true;
}

float max3(float3 val)
{
    return max(max(val.x, val.y), val.z);
}

float min3(float3 val)
{
    return min(min(val.x, val.y), val.z);
}

bool RayBounds(const __global Bounds3* bounds, float3 ray_origin, float3 ray_inv_dir, float t_min, float t_max)
{
    float3 aabb_min = bounds->pos[0];
    float3 aabb_max = bounds->pos[1];

    float3 t0 = (aabb_min - ray_origin) * ray_inv_dir;
    float3 t1 = (aabb_max - ray_origin) * ray_inv_dir;

    float tmin = max(max3(min(t0, t1)), t_min);
    float tmax = min(min3(max(t0, t1)), t_max);

    return (tmax >= tmin);
}

__kernel void TraceBvh
(
    // Input
    __global Ray* rays,
    __global uint* ray_counter,
    __global Triangle* triangles,
    __global LinearBVHNode* nodes,
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

    while (true)
    {
        __global LinearBVHNode* node = &nodes[currentNodeIndex];

        if (RayBounds(&node->bounds, ray.origin.xyz, ray_inv_dir, ray.origin.w, ray.direction.w))
        {
            // Leaf node
            if (node->nPrimitives > 0)
            {
                // Intersect ray with primitives in leaf BVH node
                for (int i = 0; i < node->nPrimitives; ++i)
                {
                    if (RayTriangle(ray, &triangles[node->offset + i], &hit.bc, &hit.t))
                    {
                        hit.primitive_id = node->offset + i;
                        // Set ray t_max
                        // TODO: remove t from hit structure
                        ray.direction.w = hit.t;

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
                if (ray_sign[node->axis])
                {
                    nodesToVisit[toVisitOffset++] = currentNodeIndex + 1;
                    currentNodeIndex = node->offset;
                }
                else
                {
                    nodesToVisit[toVisitOffset++] = node->offset;
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
    }

endtrace:
    // Write the result to the output buffer
#ifdef SHADOW_RAYS
    shadow_hits[ray_idx] = shadow_hit;
#else
    hits[ray_idx] = hit;
#endif
}
