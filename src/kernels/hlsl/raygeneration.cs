#include "common.hlsli"

cbuffer CameraData : register(b1)
{
    float4 g_CameraPositionFov;
    float4 g_CameraFrontAspect;
    float4 g_CameraUpPadding;
    uint4 g_RenderSize;
    uint4 g_SceneCounts;
};

RWStructuredBuffer<Ray> g_Rays : register(u2);
RWStructuredBuffer<uint> g_RayCounter : register(u4);
RWStructuredBuffer<uint> g_PixelIndices : register(u5);
RWStructuredBuffer<float4> g_Throughputs : register(u12);
RWStructuredBuffer<uint> g_SampleCounter : register(u15);
RWStructuredBuffer<float4> g_DiffuseAlbedo : register(u16);
RWStructuredBuffer<float> g_Depth : register(u17);
RWStructuredBuffer<float4> g_Normal : register(u18);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
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
    float3 camera_up = normalize(g_CameraUpPadding.xyz);
    float3 camera_right = normalize(cross(camera_front, camera_up));
    float3 dir = normalize(x * camera_right + y * camera_up + camera_front);

    Ray ray;
    ray.origin = float4(camera_position, 0.0f);
    ray.direction = float4(dir, MAX_RENDER_DIST);

    g_Rays[pixel_idx] = ray;
    g_PixelIndices[pixel_idx] = pixel_idx;
    g_Throughputs[pixel_idx] = float4(1.0f, 1.0f, 1.0f, 0.0f);
    g_DiffuseAlbedo[pixel_idx] = 0.0f.xxxx;
    g_Depth[pixel_idx] = MAX_RENDER_DIST;
    g_Normal[pixel_idx] = 0.0f.xxxx;
}
