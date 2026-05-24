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

struct RootConstants
{
    uint width;
    uint env_map_index;
};

ROOT_CONSTANTS
ConstantBuffer<RootConstants> g_RootConstants;

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

// Texture data
Texture2D<float4> g_TextureImages[MAX_TEXTURES] : register(t7);
SamplerState g_TextureSampler : register(s8);

ConstantBuffer<SceneInfo> g_SceneInfo : register(b24);

float3 SampleSky(float3 dir)
{
    float2 coords = float2(atan2(dir.x, dir.y) + PI, acos(clamp(dir.z, -1.0f, 1.0f)));
    coords.x = coords.x < 0.0f ? coords.x + TWO_PI : coords.x;
    coords.x *= INV_TWO_PI;
    coords.y *= INV_PI;

    uint texture_index = g_RootConstants.env_map_index;
    if (texture_index == INVALID_TEXTURE_IDX || texture_index >= g_SceneInfo.texture_count)
    {
        return float3(0.02f, 0.02f, 0.025f);
    }

    return g_TextureImages[NonUniformResourceIndex(texture_index)].SampleLevel(g_TextureSampler, coords, 0.0f).xyz;
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
    uint2 pixel_coord = PixelCoord(pixel_idx, g_RootConstants.width);
    float3 throughput = g_Throughputs[pixel_coord].xyz;
    float3 sky_radiance = SampleSky(normalize(ray.direction));

#if WHITE_FURNACE
    // Disable sky radiance but don't set it 0 to avoid changing descriptor set layout
    sky_radiance *= 1e-8f;
    sky_radiance += 0.5f;
#endif // WHITE_FURNACE

    float4 radiance = g_Radiance[pixel_coord];
    radiance.xyz += throughput * sky_radiance;
    g_Radiance[pixel_coord] = radiance;
}
