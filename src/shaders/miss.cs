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

// Ray data
RWStructuredBuffer<Ray> g_Rays : register(u6);

// Ray data
RWStructuredBuffer<uint> g_RayCounter : register(u1);
RWStructuredBuffer<uint> g_PixelIndices : register(u2);

// Hit data
RWStructuredBuffer<Hit> g_Hits : register(u3);

// Radiance and throughput data
IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_Throughputs : register(u4);
IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_Radiance : register(u5);

// Environment map data
Texture2D<float4> g_EnvMap : register(t7);
SamplerState g_EnvMapSampler : register(s8);

float3 SampleSky(float3 dir)
{
    uint width = g_RenderParams.z;
    uint height = g_RenderParams.w;
    if (width == 0u || height == 0u)
    {
        return float3(0.02f, 0.02f, 0.025f);
    }

    float2 coords = float2(atan2(dir.x, dir.y) + PI, acos(clamp(dir.z, -1.0f, 1.0f)));
    coords.x = coords.x < 0.0f ? coords.x + TWO_PI : coords.x;
    coords.x *= INV_TWO_PI;
    coords.y *= INV_PI;

    return g_EnvMap.SampleLevel(g_EnvMapSampler, coords, 0.0f).xyz;
}

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint ray_idx = dispatch_thread_id.x;
    if (ray_idx >= g_RayCounter[0])
    {
        return;
    }

    Hit hit = g_Hits[ray_idx];
    if (hit.primitive_id != INVALID_ID)
    {
        return;
    }

    Ray ray = g_Rays[ray_idx];
    uint pixel_idx = g_PixelIndices[ray_idx];
    uint2 pixel_coord = PixelCoord(pixel_idx, g_RenderSize.x);
    float3 throughput = g_Throughputs[pixel_coord].xyz;
    float3 sky_radiance = (g_RenderParams.y & RENDER_FLAG_WHITE_FURNACE) != 0u
                              ? 0.5f.xxx
                              : SampleSky(normalize(ray.direction));
    float4 radiance = g_Radiance[pixel_coord];
    radiance.xyz += throughput * sky_radiance;
    g_Radiance[pixel_coord] = radiance;
}
