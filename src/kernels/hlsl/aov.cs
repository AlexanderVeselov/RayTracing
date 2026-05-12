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
RWStructuredBuffer<Ray> g_Rays : register(u1);
RWStructuredBuffer<uint> g_RayCounter : register(u2);
RWStructuredBuffer<uint> g_PixelIndices : register(u3);

// Hit data
RWStructuredBuffer<Hit> g_Hits : register(u4);

// AOV data
RWTexture2D<float4> g_DiffuseAlbedo : register(u5);
RWTexture2D<float> g_Depth : register(u6);
RWTexture2D<float4> g_Normal : register(u7);
RWTexture2D<float4> g_MotionVectors : register(u8);

// Scene data
StructuredBuffer<Triangle> g_Triangles : register(t9);
StructuredBuffer<PackedMaterial> g_Materials : register(t10);

// Texture data
StructuredBuffer<TextureInfo> g_Textures : register(t11);
StructuredBuffer<uint> g_TextureData : register(t12);

#include "material.hlsli"

float2 ProjectScreen(float3 position, float3 camera_position, float3 camera_front, float3 camera_up,
    float fov, float aspect_ratio)
{
    float3 d = normalize(position - camera_position);
    float3 ipd = d / dot(camera_front, d);
    float angle = tan(0.5f * fov);
    float3 right = cross(camera_front, camera_up);
    float u = dot(right, ipd) / (angle * aspect_ratio);
    float v = dot(camera_up, ipd) / angle;
    return float2(u, v) * 0.5f + 0.5f;
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
    if (hit.primitive_id == INVALID_ID)
    {
        return;
    }

    uint pixel_idx = g_PixelIndices[ray_idx];
    uint2 pixel_coord = PixelCoord(pixel_idx, g_RenderSize.x);
    Ray ray = g_Rays[ray_idx];
    Triangle tri = g_Triangles[hit.primitive_id];
    float3 position = InterpolateAttributes(
        tri.v1.position.xyz, tri.v2.position.xyz, tri.v3.position.xyz, hit.bc);
    float2 texcoord =
        InterpolateAttributes2(tri.v1.texcoord.xy, tri.v2.texcoord.xy, tri.v3.texcoord.xy, hit.bc);
    float3 normal = normalize(
        InterpolateAttributes(tri.v1.normal.xyz, tri.v2.normal.xyz, tri.v3.normal.xyz, hit.bc));
    Material material = ApplyTextures(g_Materials[tri.material_index], texcoord, g_SceneCounts.w);

    g_DiffuseAlbedo[pixel_coord] = float4(material.diffuse_albedo, 1.0f);
    g_Depth[pixel_coord] = length(ray.origin.xyz - position);
    g_Normal[pixel_coord] = float4(normal, 0.0f);
    float2 current_uv =
        ProjectScreen(position, g_CameraPositionFov.xyz, normalize(g_CameraFrontAspect.xyz),
            normalize(g_CameraUpAperture.xyz), g_CameraPositionFov.w, g_CameraFrontAspect.w);
    float2 prev_uv = ProjectScreen(position, g_PrevCameraPositionFov.xyz,
        normalize(g_PrevCameraFrontAspect.xyz), normalize(g_PrevCameraUpAperture.xyz),
        g_PrevCameraPositionFov.w, g_PrevCameraFrontAspect.w);
    g_MotionVectors[pixel_coord] = float4(current_uv - prev_uv, 0.0f, 0.0f);
}
