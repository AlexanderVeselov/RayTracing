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
StructuredBuffer<float4> g_EnvMap : register(t7);

float3 LoadEnvMapTexel(int x, int y, uint width, uint height)
{
    x = (x % int(width) + int(width)) % int(width);
    y = clamp(y, 0, int(height) - 1);
    return g_EnvMap[uint(y) * width + uint(x)].xyz;
}

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

    float x = frac(coords.x) * float(width) - 0.5f;
    float y = saturate(coords.y) * float(height) - 0.5f;

    int x0 = int(floor(x));
    int y0 = int(floor(y));
    float tx = frac(x);
    float ty = frac(y);

    float3 c00 = LoadEnvMapTexel(x0, y0, width, height);
    float3 c10 = LoadEnvMapTexel(x0 + 1, y0, width, height);
    float3 c01 = LoadEnvMapTexel(x0, y0 + 1, width, height);
    float3 c11 = LoadEnvMapTexel(x0 + 1, y0 + 1, width, height);

    return lerp(lerp(c00, c10, tx), lerp(c01, c11, tx), ty);
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
