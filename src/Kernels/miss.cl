#define MAX_RENDER_DIST 20000.0f
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define INV_PI 0.31830988618f
#define INV_TWO_PI 0.15915494309f
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

float3 SampleSky(float3 dir, __read_only image2d_t tex)
{
    const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;

    // Convert (normalized) dir to spherical coordinates.
    float2 coords = (float2)(atan2(dir.x, dir.y) + PI, acos(dir.z));
    coords.x = coords.x < 0.0f ? coords.x + TWO_PI : coords.x;
    coords.x *= INV_TWO_PI;
    coords.y *= INV_PI;

    return read_imagef(tex, smp, coords).xyz;

}

__kernel void KernelEntry
(
    // Input
    __global Ray* rays,
    uint num_rays,
    __global Hit* hits,
    //TODO: throughput
    __read_only image2d_t tex,
    // Output
    __global float3* result_radiance
)
{
    uint ray_idx = get_global_id(0);

    if (ray_idx >= num_rays)
    {
        return;
    }

    Ray ray = rays[ray_idx];
    Hit hit = hits[ray_idx];

    if (hit.primitive_id == INVALID_ID)
    {
        result_radiance[ray_idx] = SampleSky(ray.direction.xyz, tex);
    }
    else
    {
        result_radiance[ray_idx] = (float3)(hit.bc, 0);
    }
}
