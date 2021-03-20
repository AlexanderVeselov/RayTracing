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

float3 SampleHemisphereCosine(float3 n, float2 s)
{
    float phi = TWO_PI * s.x;
    float sinThetaSqr = s.y;
    float sinTheta = sqrt(sinThetaSqr);

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 b = cross(n, t);

    return normalize(b * cos(phi) * sinTheta + t * sin(phi) * sinTheta + n * sqrt(1.0f - sinThetaSqr));
}

float SampleBlueNoise(int pixel_i, int pixel_j, int sampleIndex, int sampleDimension,
    __global int* sobol_256spp_256d, __global int* scramblingTile, __global int* rankingTile)
{
    // wrap arguments
    pixel_i = pixel_i & 127;
    pixel_j = pixel_j & 127;
    sampleIndex = sampleIndex & 255;
    sampleDimension = sampleDimension & 255;

    // xor index based on optimized ranking
    int rankedSampleIndex = sampleIndex ^ rankingTile[sampleDimension + (pixel_i + pixel_j * 128) * 8];

    // fetch value in sequence
    int value = sobol_256spp_256d[sampleDimension + rankedSampleIndex * 256];

    // If the dimension is optimized, xor sequence value based on optimized scrambling
    value = value ^ scramblingTile[(sampleDimension % 8) + (pixel_i + pixel_j * 128) * 8];

    // convert to float and return
    float v = (0.5f + value) / 256.0f;
    return v;
}

float3 SampleBxdf(float2 s, float3 normal, float3* outgoing, float* pdf)
{
    *outgoing = SampleHemisphereCosine(normal, s);
    *pdf = 1.0f;

    return (float3)(1.0f, 1.0f, 1.0f);
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
    uint bounce,
    uint width,
    uint height,
    // Sampler
    __global int* sobol_256spp_256d,
    __global int* scramblingTile,
    __global int* rankingTile,
    // Output
    __global float3* throughputs,
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

    int x = pixel_idx % width;
    int y = pixel_idx / width;

    Triangle triangle = triangles[hit.primitive_id];

    float3 position = InterpolateAttributes(triangle.v1.position,
        triangle.v2.position, triangle.v3.position, hit.bc);

    float3 normal = normalize(InterpolateAttributes(triangle.v1.normal,
        triangle.v2.normal, triangle.v3.normal, hit.bc));

    // Sample bxdf
    float2 s;
    s.x = SampleBlueNoise(x, y, 0, 0, sobol_256spp_256d, scramblingTile, rankingTile);
    s.y = SampleBlueNoise(x, y, 0, 1, sobol_256spp_256d, scramblingTile, rankingTile);

    float pdf = 0.0f;
    float3 throughput;
    float3 outgoing;
    float3 bxdf = SampleBxdf(s, normal, &outgoing, &pdf);

    if (pdf > 0.0)
    {
        float3 throughput = bxdf / pdf * max(dot(outgoing, normal), 0.0f);
    }

    bool spawn_outgoing_ray = (pdf > 0.0);

    if (spawn_outgoing_ray)
    {

        ///@TODO: reduct atomic memory traffic by using LDS
        uint outgoing_ray_idx = atomic_add(outgoing_ray_counter, 1);

        Ray outgoing_ray;
        outgoing_ray.origin.xyz = position + normal * EPS;
        outgoing_ray.origin.w = 0.0f;
        outgoing_ray.direction.xyz = outgoing;
        outgoing_ray.direction.w = MAX_RENDER_DIST;

        outgoing_rays[outgoing_ray_idx] = outgoing_ray;
        outgoing_pixel_indices[outgoing_ray_idx] = pixel_idx;
    }

}
