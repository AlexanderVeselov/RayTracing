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
RWStructuredBuffer<Ray>    g_Rays                 : register(u1);
RWStructuredBuffer<uint>   g_RayCounter           : register(u2);
RWStructuredBuffer<uint>   g_PixelIndices         : register(u3);

// Radiance and throughput data
RWStructuredBuffer<float4> g_Throughputs          : register(u4);

// Sample counter data
RWStructuredBuffer<uint>   g_SampleCounter        : register(u5);

// AOV data
RWStructuredBuffer<float4> g_DiffuseAlbedo        : register(u6);
RWStructuredBuffer<float>  g_Depth                : register(u7);
RWStructuredBuffer<float4> g_Normal               : register(u8);
RWStructuredBuffer<float4> g_MotionVectors        : register(u9);

float2 PointInHexagon(inout uint seed)
{
    float2 hex_points[3] = { float2(-1.0f, 0.0f), float2(0.5f, 0.866f), float2(0.5f, -0.866f) };
    uint x = min(uint(RandomFloat(seed) * 3.0f), 2u);
    float2 v1 = hex_points[x];
    float2 v2 = hex_points[(x + 1u) % 3u];
    float p1 = RandomFloat(seed);
    float p2 = RandomFloat(seed);
    return p1 * v1 + p2 * v2;
}

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

    if (pixel_idx == 0)
    {
        g_RayCounter[0] = num_pixels;
    }

    uint pixel_x = pixel_idx % width;
    uint pixel_y = pixel_idx / width;
    uint seed = pixel_idx + WangHash(g_SampleCounter[0]);

    float x = (float(pixel_x) + RandomFloat(seed)) / float(width);
    float y = (float(pixel_y) + RandomFloat(seed)) / float(height);
    float angle = tan(0.5f * g_CameraPositionFov.w);
    x = (x * 2.0f - 1.0f) * angle * g_CameraFrontAspect.w;
    y = -((y * 2.0f - 1.0f) * angle);

    float3 camera_position = g_CameraPositionFov.xyz;
    float3 camera_front = normalize(g_CameraFrontAspect.xyz);
    float3 camera_up = normalize(g_CameraUpAperture.xyz);
    float3 camera_right = normalize(cross(camera_front, camera_up));
    float3 dir = normalize(x * camera_right + y * camera_up + camera_front);

    float3 point_aimed = camera_position + g_CameraLens.x * dir;
    float2 dof_dir = PointInHexagon(seed);
    float aperture = g_CameraUpAperture.w;
    float3 ray_origin =
        camera_position + dof_dir.x * aperture * camera_right + dof_dir.y * aperture * camera_up;

    Ray ray;
    ray.origin = float4(ray_origin, 0.0f);
    ray.direction = float4(normalize(point_aimed - ray_origin), MAX_RENDER_DIST);

    g_Rays[pixel_idx] = ray;
    g_PixelIndices[pixel_idx] = pixel_idx;
    g_Throughputs[pixel_idx] = float4(1.0f, 1.0f, 1.0f, 0.0f);
    g_DiffuseAlbedo[pixel_idx] = 0.0f.xxxx;
    g_Depth[pixel_idx] = MAX_RENDER_DIST;
    g_Normal[pixel_idx] = 0.0f.xxxx;
    g_MotionVectors[pixel_idx] = 0.0f.xxxx;
}
