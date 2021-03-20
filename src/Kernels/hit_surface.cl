#include "shared_structures.h"

#define MAX_RENDER_DIST 20000.0f
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define INV_PI 0.31830988618f
#define INV_TWO_PI 0.15915494309f
#define INVALID_ID 0xFFFFFFFF

float3 InterpolateAttributes(float3 attr1, float3 attr2, float3 attr3, float2 uv)
{
    return attr1 * (1.0f - uv.x - uv.y) + attr2 * uv.x + attr3 * uv.y;
}

__kernel void KernelEntry
(
    // Input
    __global Ray* rays,
    uint num_rays,
    __global Hit* hits,
    __global Triangle* triangles,
    __global Material* materials,
    // Output
    __global float3* result_radiance
)
{
    uint ray_idx = get_global_id(0);

    if (ray_idx >= num_rays)
    {
        return;
    }

    Hit hit = hits[ray_idx];

    if (hit.primitive_id == INVALID_ID)
    {
        return;
    }

    Ray incoming_ray = rays[ray_idx];
    Triangle triangle = triangles[hit.primitive_id];

    float3 normal = normalize(InterpolateAttributes(triangle.v1.normal,
        triangle.v2.normal, triangle.v3.normal, hit.bc));

    result_radiance[ray_idx] = normal;
}
