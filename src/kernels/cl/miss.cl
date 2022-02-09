/*****************************************************************************
 MIT License

 Copyright(c) 2022 Alexander Veselov

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

#include "../shared_structures.h"
#include "../constants.h"

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

__kernel void Miss
(
    // Input
    __global Ray* rays,
    __global uint* ray_counter,
    __global Hit* hits,
    __global uint* pixel_indices,
    __global float3* throughputs,
    __read_only image2d_t tex,
    // Output
    __global float3* result_radiance
)
{
    uint ray_idx = get_global_id(0);
    uint num_rays = ray_counter[0];

    if (ray_idx >= num_rays)
    {
        return;
    }

    Ray ray = rays[ray_idx];
    Hit hit = hits[ray_idx];

    if (hit.primitive_id == INVALID_ID)
    {
        uint pixel_idx = pixel_indices[ray_idx];
        float3 throughput = throughputs[pixel_idx];

#ifdef ENABLE_WHITE_FURNACE
        float3 sky_radiance = 0.5f;
#else
        float3 sky_radiance = SampleSky(ray.direction.xyz, tex);
#endif
        result_radiance[pixel_idx] += sky_radiance * throughput;
    }
}
