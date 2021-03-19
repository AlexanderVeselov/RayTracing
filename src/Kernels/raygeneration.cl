#include "src/utils/shared_structs.hpp"

#define MAX_RENDER_DIST 20000.0f
#define INVALID_ID 0xFFFFFFFF

typedef struct
{
    float4 origin; // w - t_min
    float4 direction; // w - t_max
} Ray;

float GetRandomFloat(unsigned int* seed)
{
    *seed = (*seed ^ 61) ^ (*seed >> 16);
    *seed = *seed + (*seed << 3);
    *seed = *seed ^ (*seed >> 4);
    *seed = *seed * 0x27d4eb2d;
    *seed = *seed ^ (*seed >> 15);
    *seed = 1103515245 * (*seed) + 12345;

    return (float)(*seed) * 2.3283064365386963e-10f;
}

float2 PointInHexagon(unsigned int* seed)
{
    float2 hexPoints[3] = { (float2)(-1.0f, 0.0f), (float2)(0.5f, 0.866f), (float2)(0.5f, -0.866f) };
    int x = floor(GetRandomFloat(seed) * 3.0f);
    float2 v1 = hexPoints[x];
    float2 v2 = hexPoints[(x + 1) % 3];
    float p1 = GetRandomFloat(seed);
    float p2 = GetRandomFloat(seed);
    return (float2)(p1 * v1.x + p2 * v2.x, p1 * v1.y + p2 * v2.y);
}

unsigned int HashUInt32(unsigned int x)
{
#if 0
    x = (x ^ 61) ^ (x >> 16);
    x = x + (x << 3);
    x = x ^ (x >> 4);
    x = x * 0x27d4eb2d;
    x = x ^ (x >> 15);
    return x;
#else
    return 1103515245 * x + 12345;
#endif
}

__kernel void KernelEntry
(
    // Input
    uint width,
    uint height,
    float3 cameraPos,
    float3 cameraFront,
    float3 cameraUp,
    unsigned int frameCount,
    unsigned int frameSeed,
    float aperture,
    float focus_distance,
    // Output
    __global Ray* rays
)
{
    uint ray_idx = get_global_id(0);

    if (ray_idx >= width * height)
    {
        return;
    }

    unsigned int seed = ray_idx + HashUInt32(frameCount);

    uint pixel_x = ray_idx % width;
    uint pixel_y = ray_idx / width;

    float invWidth = 1.0f / (float)(width);
    float invHeight = 1.0f / (float)(height);
    float aspectratio = (float)(width) / (float)(height);
    float fov = 45.0f * 3.1415f / 180.0f;
    float angle = tan(0.5f * fov);

    float x = (float)(get_global_id(0) % width) + GetRandomFloat(&seed) - 0.5f;
    float y = (float)(get_global_id(0) / width) + GetRandomFloat(&seed) - 0.5f;

    x = (2.0f * ((x + 0.5f) * invWidth) - 1) * angle * aspectratio;
    y = -(1.0f - 2.0f * ((y + 0.5f) * invHeight)) * angle;

    float3 dir = normalize(x * cross(cameraFront, cameraUp) + y * cameraUp + cameraFront);

    // Simple Depth of Field
    float3 pointAimed = cameraPos + focus_distance * dir;
    float2 dofDir = PointInHexagon(&seed);
    float r = aperture;
    float3 newPos = cameraPos + dofDir.x * r * cross(cameraFront, cameraUp) + dofDir.y * r * cameraUp;

    Ray ray;
    ray.origin.xyz = newPos;
    ray.origin.w = 0.0;
    ray.direction.xyz = normalize(pointAimed - newPos);
    ray.direction.w = MAX_RENDER_DIST;

    rays[ray_idx] = ray;
}
