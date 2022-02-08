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

#include "../constants.h"

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
