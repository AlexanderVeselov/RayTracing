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

// Radiance history data
IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_Radiance : register(u0);
IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_PrevRadiance : register(u1);

// Geometry history data
RWTexture2D<float> g_Depth : register(u2);
RWTexture2D<float> g_PrevDepth : register(u3);

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint width;
    uint height;
    g_Radiance.GetDimensions(width, height);
    if (dispatch_thread_id.x >= width || dispatch_thread_id.y >= height)
    {
        return;
    }

    uint2 pixel_coord = dispatch_thread_id.xy;
    g_PrevRadiance[pixel_coord] = g_Radiance[pixel_coord];
    g_PrevDepth[pixel_coord] = g_Depth[pixel_coord];
}
