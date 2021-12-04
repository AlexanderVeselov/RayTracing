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

__kernel void TemporalFilter
(
    uint width,
    uint height,
    __global float4* radiance_buffer,
    __global float4* prev_radiance_buffer,
    __global float4* accumulated_radiance_buffer,
    __global float*  depth_buffer,
    __global float*  prev_depth_buffer,
    __global float2* motion_vectors_buffer
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

    float2 motion = motion_vectors_buffer[pixel_idx];
    float2 prev_uv = screen_uv - motion;
    int prev_x = prev_uv.x * width;
    int prev_y = prev_uv.y * height;

    bool valid_history = true;

    if (prev_x < 0 || prev_x >= width || prev_y < 0 || prev_y >= height)
    {
        return; //valid_history = false;
    }

    int prev_idx = prev_y * width + prev_x;
    float prev_depth = prev_depth_buffer[prev_idx];

    // Depth similarity test
    if (fabs(depth - prev_depth) / depth > 0.1f)
    {
        return;//valid_history = false;
    }

    float3 current_radiance = radiance_buffer[pixel_idx].xyz;
    float3 prev_radiance = SampleBicubic(prev_uv, prev_radiance_buffer, width, height).xyz;

    accumulated_radiance_buffer[pixel_idx].xyz = mix(current_radiance, prev_radiance, 0.9f);

}
