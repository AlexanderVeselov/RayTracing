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

#include "constants.h"
#include "utils.h"

__kernel void SpatialFilter
(
    uint width,
    uint height,
    __global float4* input_radiance_buffer,
    __global float4* output_radiance_buffer,
    __global float*  depth_buffer,
    __global float3* normal_buffer
)
{
    uint pixel_idx = get_global_id(0);

    int x = pixel_idx % width;
    int y = pixel_idx / width;

    if (x >= width || y >= height)
    {
        return;
    }

    float depth = depth_buffer[pixel_idx];

    if (depth == MAX_RENDER_DIST)
    {
        // Background
        return;
    }

    float2 screen_uv = (float2)(x + 0.5f, y + 0.5f) / (float2)(width, height);

    float3 avg_radiance = 0.0f;
    float weight_sum = 0.0f;
    int kRadius = 3;

    float3 center_normal = normal_buffer[pixel_idx];

    for (int dx = -kRadius; dx <= kRadius; ++dx)
    {
        for (int dy = -kRadius; dy <= kRadius; ++dy)
        {
            int xx = x + dx;
            int yy = y + dy;

            if (xx < 0 || xx >= width || yy < 0 || yy >= height)
            {
                continue;
            }

            int current_pixel_idx = To1D((int2)(xx, yy), width);
            float3 current_radiance = input_radiance_buffer[current_pixel_idx].xyz;
            float3 current_normal = normal_buffer[current_pixel_idx];

            float weight = pow(max(dot(current_normal, center_normal), 0.0f), 5.0f);
            avg_radiance += current_radiance * weight;
            weight_sum += weight;
        }
    }

    output_radiance_buffer[pixel_idx].xyz = weight_sum > 0.0f ? avg_radiance / weight_sum : 0.0f;
}
