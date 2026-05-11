#include "material.hlsli"

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
RWStructuredBuffer<Hit> g_Hits : register(u7);
RWStructuredBuffer<float4> g_DiffuseAlbedo : register(u16);
RWStructuredBuffer<float> g_Depth : register(u17);
RWStructuredBuffer<float4> g_Normal : register(u18);
StructuredBuffer<RhiTriangle> g_Triangles : register(t20);
StructuredBuffer<PackedMaterial> g_Materials : register(t22);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
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
    Ray ray = g_Rays[ray_idx];
    RhiTriangle tri = g_Triangles[hit.primitive_id];
    float3 position = InterpolateAttributes(tri.v1.position.xyz, tri.v2.position.xyz,
        tri.v3.position.xyz, hit.bc);
    float2 texcoord = InterpolateAttributes2(tri.v1.texcoord.xy, tri.v2.texcoord.xy,
        tri.v3.texcoord.xy, hit.bc);
    float3 normal = normalize(InterpolateAttributes(tri.v1.normal.xyz, tri.v2.normal.xyz,
        tri.v3.normal.xyz, hit.bc));
    Material material = ApplyTextures(g_Materials[tri.material_index], texcoord, g_SceneCounts.w);

    g_DiffuseAlbedo[pixel_idx] = float4(material.diffuse_albedo, 1.0f);
    g_Depth[pixel_idx] = length(ray.origin.xyz - position);
    g_Normal[pixel_idx] = float4(normal, 0.0f);
}
