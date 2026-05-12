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

// Incoming ray data
RWStructuredBuffer<Ray> g_IncomingRays : register(u1);
RWStructuredBuffer<uint> g_IncomingPixelIndices : register(u2);
RWStructuredBuffer<uint> g_IncomingRayCounter : register(u3);

// Outgoing ray data
RWStructuredBuffer<Ray> g_OutgoingRays : register(u4);
RWStructuredBuffer<uint> g_OutgoingPixelIndices : register(u5);
RWStructuredBuffer<uint> g_OutgoingRayCounter : register(u6);

// Hit data
RWStructuredBuffer<Hit> g_Hits : register(u7);

// Shadow ray data
RWStructuredBuffer<Ray> g_ShadowRays : register(u8);
RWStructuredBuffer<uint> g_ShadowPixelIndices : register(u9);
RWStructuredBuffer<uint> g_ShadowRayCounter : register(u10);
RWTexture2D<float4> g_DirectLightSamples : register(u11);

// Radiance and throughput data
RWTexture2D<float4> g_Throughputs : register(u12);
RWTexture2D<float4> g_Radiance : register(u13);

// Bounce and sample counters
StructuredBuffer<uint> g_Bounce : register(t14);
RWStructuredBuffer<uint> g_SampleCounter : register(u15);

// Scene data
StructuredBuffer<Triangle> g_Triangles : register(t16);
StructuredBuffer<PackedMaterial> g_Materials : register(t17);
StructuredBuffer<Light> g_Lights : register(t18);

// Texture data
StructuredBuffer<TextureInfo> g_Textures : register(t19);
StructuredBuffer<uint> g_TextureData : register(t20);

#include "light.hlsli"
#include "material.hlsli"

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint ray_idx = dispatch_thread_id.x;
    if (ray_idx >= g_IncomingRayCounter[0])
    {
        return;
    }

    Hit hit = g_Hits[ray_idx];
    if (hit.primitive_id == INVALID_ID)
    {
        return;
    }

    uint pixel_idx = g_IncomingPixelIndices[ray_idx];
    uint sample_idx = g_SampleCounter[0];
    uint pixel_x = pixel_idx % g_RenderSize.x;
    uint pixel_y = pixel_idx / g_RenderSize.x;
    uint2 pixel_coord = uint2(pixel_x, pixel_y);
    uint bounce = g_Bounce[0];

    Ray incoming_ray = g_IncomingRays[ray_idx];
    float3 incoming = -incoming_ray.direction.xyz;

    Triangle tri = g_Triangles[hit.primitive_id];
    float3 position = InterpolateAttributes(
        tri.v1.position.xyz, tri.v2.position.xyz, tri.v3.position.xyz, hit.bc);
    float2 texcoord =
        InterpolateAttributes2(tri.v1.texcoord.xy, tri.v2.texcoord.xy, tri.v3.texcoord.xy, hit.bc);
    float3 geometry_normal = normalize(cross(
        tri.v2.position.xyz - tri.v1.position.xyz, tri.v3.position.xyz - tri.v1.position.xyz));
    float3 normal = normalize(
        InterpolateAttributes(tri.v1.normal.xyz, tri.v2.normal.xyz, tri.v3.normal.xyz, hit.bc));
    if (dot(normal, incoming) < 0.0f)
    {
        normal = -normal;
    }
    if (dot(geometry_normal, normal) < 0.0f)
    {
        geometry_normal = -geometry_normal;
    }

    Material material = ApplyTextures(g_Materials[tri.material_index], texcoord, g_SceneCounts.w);
    float3 throughput = g_Throughputs[pixel_coord].xyz;
    if ((g_RenderParams.y & RENDER_FLAG_WHITE_FURNACE) == 0u &&
        dot(material.emission, 1.0f.xxx) > 0.0f)
    {
        float4 radiance = g_Radiance[pixel_coord];
        radiance.xyz += throughput * material.emission;
        g_Radiance[pixel_coord] = radiance;
    }

    // Direct lighting
    {
        float s_light = SampleRandom(pixel_x, pixel_y, sample_idx, bounce, SAMPLE_TYPE_LIGHT);
        float3 outgoing;
        float pdf;
        float3 light_radiance = Light_Sample(position, normal, s_light, outgoing, pdf);

        float distance_to_light = length(outgoing);
        outgoing = normalize(outgoing);

        float3 brdf = EvaluateMaterial(material, normal, incoming, outgoing);
        float3 light_sample =
            pdf > 0.0f ? light_radiance * throughput * brdf / pdf * max(dot(outgoing, normal), 0.0f)
                       : 0.0f.xxx;

        bool spawn_shadow_ray = pdf > 0.0f && dot(light_sample, light_sample) > 0.0f;

        if (spawn_shadow_ray)
        {
            Ray shadow_ray;
            shadow_ray.origin = float4(position + normal * EPS, 0.0f);
            shadow_ray.direction = float4(outgoing, distance_to_light);

            uint shadow_ray_idx;
            InterlockedAdd(g_ShadowRayCounter[0], 1, shadow_ray_idx);

            g_ShadowRays[shadow_ray_idx] = shadow_ray;
            g_ShadowPixelIndices[shadow_ray_idx] = pixel_idx;
            g_DirectLightSamples[PixelCoord(shadow_ray_idx, g_RenderSize.x)] =
                float4(light_sample, 0.0f);
        }
    }

    // Indirect lighting
    {
        float2 s;
        s.x = SampleRandom(pixel_x, pixel_y, sample_idx, bounce, SAMPLE_TYPE_BXDF_U);
        s.y = SampleRandom(pixel_x, pixel_y, sample_idx, bounce, SAMPLE_TYPE_BXDF_V);
        float s1 = SampleRandom(pixel_x, pixel_y, sample_idx, bounce, SAMPLE_TYPE_BXDF_LAYER);

        float pdf = 0.0f;
        float3 indirect_throughput = 0.0f.xxx;
        float3 outgoing;
        float offset;
        float3 bxdf = SampleBxdf(s1, s, material, normal, incoming, outgoing, pdf, offset);

        if (pdf > 0.0f)
        {
            indirect_throughput = bxdf / pdf;
        }

        g_Throughputs[pixel_coord] = float4(throughput * indirect_throughput, 0.0f);

        bool spawn_outgoing_ray = pdf > 0.0f;

        if (spawn_outgoing_ray)
        {
            uint outgoing_ray_idx;
            InterlockedAdd(g_OutgoingRayCounter[0], 1, outgoing_ray_idx);

            Ray outgoing_ray;
            outgoing_ray.origin = float4(position + geometry_normal * EPS * offset, 0.0f);
            outgoing_ray.direction = float4(outgoing, MAX_RENDER_DIST);

            g_OutgoingRays[outgoing_ray_idx] = outgoing_ray;
            g_OutgoingPixelIndices[outgoing_ray_idx] = pixel_idx;
        }
    }
}
