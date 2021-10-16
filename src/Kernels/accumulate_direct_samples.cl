#include "constants.h"

__kernel void AccumulateDirectSamples
(
    // Input
    __global uint* shadow_hits,
    __global uint* shadow_ray_counter,
    __global uint* shadow_pixel_indices,
    __global float3* direct_light_samples,
    // Output
    __global float4* result_radiance
)
{
    uint ray_idx = get_global_id(0);
    uint num_rays = shadow_ray_counter[0];

    if (ray_idx >= num_rays)
    {
        return;
    }

    uint shadow_hit = shadow_hits[ray_idx];

    if (shadow_hit == INVALID_ID)
    {
        uint pixel_idx = shadow_pixel_indices[ray_idx];
        result_radiance[pixel_idx].xyz += direct_light_samples[ray_idx];
    }
}
