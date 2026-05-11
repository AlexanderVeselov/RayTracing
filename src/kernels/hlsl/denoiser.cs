/*****************************************************************************
 MIT License

 Copyright(c) 2026 Alexander Veselov

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

#include "common.hlsli"
#include "frame_data.hlsli"

// Radiance history data
RWStructuredBuffer<float4> g_Radiance             : register(u1);
StructuredBuffer<float4>   g_PrevRadiance         : register(t2);

// Geometry history data
RWStructuredBuffer<float>  g_Depth                : register(u3);
StructuredBuffer<float>    g_PrevDepth            : register(t4);
RWStructuredBuffer<float4> g_MotionVectors        : register(u5);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint pixel_idx = dispatch_thread_id.x;
    uint width = g_RenderSize.x;
    uint height = g_RenderSize.y;
    uint num_pixels = width * height;
    if (pixel_idx >= num_pixels)
    {
        return;
    }
    uint x = pixel_idx % width;
    uint y = pixel_idx / width;
    float depth_value = g_Depth[pixel_idx];
    if (depth_value == MAX_RENDER_DIST)
    {
        return;
    }

    float2 motion = g_MotionVectors[pixel_idx].xy;
    float2 prev_uv = (float2(float(x) + 0.5f, float(y) + 0.5f) / float2(width, height)) - motion;
    int prev_x = int(prev_uv.x * float(width));
    int prev_y = int(prev_uv.y * float(height));
    if (prev_x < 0 || prev_x >= int(width) || prev_y < 0 || prev_y >= int(height))
    {
        return;
    }

    uint prev_idx = uint(prev_y) * width + uint(prev_x);
    float prev_depth_value = g_PrevDepth[prev_idx];
    if (abs(depth_value - prev_depth_value) / max(depth_value, EPS) > 0.1f)
    {
        return;
    }

    float3 current_radiance = g_Radiance[pixel_idx].xyz;
    float3 prev_radiance = g_PrevRadiance[prev_idx].xyz;
    g_Radiance[pixel_idx].xyz = lerp(current_radiance, prev_radiance, 0.9f);
}
