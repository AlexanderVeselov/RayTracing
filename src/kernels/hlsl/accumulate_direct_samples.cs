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

// Shadow ray data
RWStructuredBuffer<uint> g_ShadowRayCounter : register(u0);
RWStructuredBuffer<uint> g_ShadowPixelIndices : register(u1);
RWStructuredBuffer<uint> g_ShadowHits : register(u2);
IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_DirectLightSamples : register(u4);

// Radiance data
IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_Radiance : register(u3);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint ray_idx = dispatch_thread_id.x;
    if (ray_idx >= g_ShadowRayCounter[0])
    {
        return;
    }

    if (g_ShadowHits[ray_idx] == INVALID_ID)
    {
        uint width;
        uint height;
        g_Radiance.GetDimensions(width, height);
        uint pixel_idx = g_ShadowPixelIndices[ray_idx];
        uint2 pixel_coord = PixelCoord(pixel_idx, width);
        float4 radiance = g_Radiance[pixel_coord];
        radiance.xyz += g_DirectLightSamples[PixelCoord(ray_idx, width)].xyz;
        g_Radiance[pixel_coord] = radiance;
    }
}
