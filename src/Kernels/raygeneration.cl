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

__kernel void RayGeneration
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
    __global Ray*    rays,
    __global uint*   ray_counter,
    __global uint*   pixel_indices,
    __global float3* throughputs,
    __global float3* diffuse_albedo,
    __global float*  depth,
    __global float2* velocity
)
{
    uint ray_idx = get_global_id(0);

    if (ray_idx >= width * height)
    {
        return;
    }

    uint pixel_idx = ray_idx;
    uint pixel_x = pixel_idx % width;
    uint pixel_y = pixel_idx / width;

    float invWidth = 1.0f / (float)(width);
    float invHeight = 1.0f / (float)(height);
    float aspectratio = (float)(width) / (float)(height);
    float fov = 75.0f * 3.1415f / 180.0f;
    float angle = tan(0.5f * fov);

    unsigned int seed = pixel_idx + HashUInt32(frameCount);

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
    pixel_indices[ray_idx] = pixel_idx;
    throughputs[pixel_idx] = (float3)(1.0f, 1.0f, 1.0f);
    diffuse_albedo[pixel_idx] = (float3)(0.0f, 0.0f, 0.0f);
    depth[pixel_idx] = MAX_RENDER_DIST;
    velocity[pixel_idx] = (float2)(0.0f, 0.0f);

    // Write to global ray counter
    if (ray_idx == 0)
    {
        ray_counter[0] = width * height;
    }
}
