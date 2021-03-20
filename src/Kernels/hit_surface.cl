#include "shared_structures.h"

#define MAX_RENDER_DIST 20000.0f
#define EPS 1e-3f
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define INV_PI 0.31830988618f
#define INV_TWO_PI 0.15915494309f
#define INVALID_ID 0xFFFFFFFF

float3 InterpolateAttributes(float3 attr1, float3 attr2, float3 attr3, float2 uv)
{
    return attr1 * (1.0f - uv.x - uv.y) + attr2 * uv.x + attr3 * uv.y;
}

float3 reflect(float3 v, float3 n)
{
    return -v + 2.0f * dot(v, n) * n;
}

__kernel void KernelEntry
(
    // Input
    __global Ray* incoming_rays,
    __global uint* incoming_ray_counter,
    __global uint* incoming_pixel_indices,
    __global Hit* hits,
    __global Triangle* triangles,
    __global Material* materials,
    // Output
    __global Ray* outgoing_rays,
    __global uint* outgoing_ray_counter,
    __global uint* outgoing_pixel_indices,
    __global float4* result_radiance
)
{
    uint incoming_ray_idx = get_global_id(0);
    uint num_incoming_rays = incoming_ray_counter[0];

    if (incoming_ray_idx >= num_incoming_rays)
    {
        return;
    }

    Hit hit = hits[incoming_ray_idx];

    if (hit.primitive_id == INVALID_ID)
    {
        return;
    }

    Ray incoming_ray = incoming_rays[incoming_ray_idx];
    uint pixel_idx = incoming_pixel_indices[incoming_ray_idx];

    Triangle triangle = triangles[hit.primitive_id];

    float3 position = InterpolateAttributes(triangle.v1.position,
        triangle.v2.position, triangle.v3.position, hit.bc);

    float3 normal = normalize(InterpolateAttributes(triangle.v1.normal,
        triangle.v2.normal, triangle.v3.normal, hit.bc));

    result_radiance[pixel_idx] += (float4)(normal, 1.0f);

    bool spawn_outgoing_ray = true;

    if (spawn_outgoing_ray)
    {
        ///@TODO: reduct atomic memory traffic by using LDS
        uint outgoing_ray_idx = atomic_add(outgoing_ray_counter, 1);

        Ray outgoing_ray;
        outgoing_ray.origin.xyz = position + normal * EPS;
        outgoing_ray.origin.w = 0.0f;
        outgoing_ray.direction.xyz = reflect(-incoming_ray.direction.xyz, normal);
        outgoing_ray.direction.w = MAX_RENDER_DIST;

        outgoing_rays[outgoing_ray_idx] = outgoing_ray;
        outgoing_pixel_indices[outgoing_ray_idx] = pixel_idx;
    }

}
