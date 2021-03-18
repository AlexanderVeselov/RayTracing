#include "src/utils/shared_structs.hpp"

#define MAX_RENDER_DIST 20000.0f
#define INVALID_ID 0xFFFFFFFF

typedef struct
{
    float4 origin; // w - t_min
    float4 direction; // w - t_max
} Ray;

typedef struct
{
    float2 bc;
    uint primitive_id;
    // TODO: remove t from hit structure
    float t;
} Hit;

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

bool RayBounds(const __global Bounds3* bounds, Ray ray)
{
    float t_min = ray.origin.w;
    float t_max = ray.direction.w;

    float t0 = max(t_min, (bounds->pos[ray.sign[0]].x - ray.origin.x) * ray.invDir.x);
    float t1 = min(t_max, (bounds->pos[1 - ray.sign[0]].x - ray.origin.x) * ray.invDir.x);

    t0 = max(t0, (bounds->pos[ray.sign[1]].y - ray.origin.y) * ray.invDir.y);
    t1 = min(t1, (bounds->pos[1 - ray.sign[1]].y - ray.origin.y) * ray.invDir.y);

    t0 = max(t0, (bounds->pos[ray.sign[2]].z - ray.origin.z) * ray.invDir.z);
    t1 = min(t1, (bounds->pos[1 - ray.sign[2]].z - ray.origin.z) * ray.invDir.z);

    return (t1 >= t0);

}

__kernel void KernelEntry
(
    // Input
    __global Ray* rays,
    uint num_rays,
    __global Triangle* triangles,
    __global LinearBVHNode* nodes,
    // Output
    __global Hit* hits
)
{
    uint ray_idx = get_global_id(0);

    if (ray_idx >= num_rays)
    {
        return;
    }

    Ray ray = rays[ray_idx];
    float3 ray_inv_dir;


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

        if (RayBounds(&node->bounds, ray))
        {
            // Leaf node
            if (node->nPrimitives > 0)
            {
                // Intersect ray with primitives in leaf BVH node
                for (int i = 0; i < node->nPrimitives; ++i)
                {
                    if (RayTriangle(ray, &scene->triangles[node->offset + i], &hit.bc, &hit.t))
                    {
                        hit.primitive_id = node->offset + i;
                        // TODO: remove t from hit structure
                        ray.t_max = hit.t;
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
                if (ray.sign[node->axis])
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

    // Write the result to the output buffer
    hits[ray_idx] = hit;
}
