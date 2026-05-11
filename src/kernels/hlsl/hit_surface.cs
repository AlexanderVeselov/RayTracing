#define TEXTURES_REGISTER t19
#define TEXTURE_DATA_REGISTER t20

#include "material.hlsli"

RWStructuredBuffer<Ray> g_IncomingRays : register(u1);
RWStructuredBuffer<Ray> g_OutgoingRays : register(u2);
RWStructuredBuffer<uint> g_IncomingRayCounter : register(u3);
RWStructuredBuffer<uint> g_IncomingPixelIndices : register(u4);
RWStructuredBuffer<uint> g_OutgoingPixelIndices : register(u5);
RWStructuredBuffer<Hit> g_Hits : register(u6);
RWStructuredBuffer<Ray> g_ShadowRays : register(u7);
RWStructuredBuffer<uint> g_ShadowRayCounter : register(u8);
RWStructuredBuffer<uint> g_ShadowPixelIndices : register(u9);
RWStructuredBuffer<float4> g_Throughputs : register(u10);
RWStructuredBuffer<float4> g_Radiance : register(u11);
RWStructuredBuffer<float4> g_DirectLightSamples : register(u12);
RWStructuredBuffer<uint> g_SampleCounter : register(u13);
RWStructuredBuffer<uint> g_OutgoingRayCounter : register(u14);
StructuredBuffer<RhiTriangle> g_Triangles : register(t15);
StructuredBuffer<PackedMaterial> g_Materials : register(t16);
StructuredBuffer<Light> g_Lights : register(t17);
StructuredBuffer<uint> g_Bounce : register(t18);

float3 SampleAnalyticLight(
    float3 position, float s, out float3 outgoing, out float pdf, out float distance_to_light)
{
    if (g_SceneCounts.z == 0)
    {
        outgoing = 0.0f.xxx;
        pdf = 0.0f;
        distance_to_light = 0.0f;
        return 0.0f.xxx;
    }

    uint light_idx = min(uint(s * float(g_SceneCounts.z)), g_SceneCounts.z - 1u);
    Light light = g_Lights[light_idx];
    pdf = 1.0f / float(g_SceneCounts.z);
    float3 light_radiance = light.radiance.xyz;

    if (light.type == LIGHT_TYPE_POINT)
    {
        outgoing = light.origin.xyz - position;
        float sq_length = max(dot(outgoing, outgoing), EPS);
        distance_to_light = sqrt(sq_length);
        return light_radiance / sq_length;
    }

    outgoing = light.origin.xyz * MAX_RENDER_DIST;
    distance_to_light = MAX_RENDER_DIST;
    return light_radiance;
}

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
    uint bounce = g_Bounce[0];
    Ray incoming_ray = g_IncomingRays[ray_idx];
    float3 incoming = -incoming_ray.direction.xyz;
    RhiTriangle tri = g_Triangles[hit.primitive_id];
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
    float3 throughput = g_Throughputs[pixel_idx].xyz;
    if ((g_RenderParams.y & RENDER_FLAG_WHITE_FURNACE) == 0u &&
        dot(material.emission, 1.0f.xxx) > 0.0f)
    {
        g_Radiance[pixel_idx].xyz += throughput * material.emission;
    }

    float3 light_outgoing;
    float light_pdf;
    float distance_to_light;
    float s_light = SampleRandom(pixel_x, pixel_y, sample_idx, bounce, SAMPLE_TYPE_LIGHT);
    float3 light_radiance =
        SampleAnalyticLight(position, s_light, light_outgoing, light_pdf, distance_to_light);
    float3 outgoing = normalize(light_outgoing);
    float3 brdf = EvaluateMaterial(material, normal, incoming, outgoing);
    float3 light_sample = light_pdf > 0.0f ? light_radiance * throughput * brdf / light_pdf *
                                                 max(dot(outgoing, normal), 0.0f)
                                           : 0.0f.xxx;
    bool spawn_shadow_ray = light_pdf > 0.0f && dot(light_sample, light_sample) > 0.0f;

    if (spawn_shadow_ray)
    {
        uint shadow_idx;
        InterlockedAdd(g_ShadowRayCounter[0], 1, shadow_idx);
        Ray shadow_ray;
        shadow_ray.origin = float4(position + normal * EPS, 0.0f);
        shadow_ray.direction = float4(outgoing, distance_to_light);
        g_ShadowRays[shadow_idx] = shadow_ray;
        g_ShadowPixelIndices[shadow_idx] = pixel_idx;
        g_DirectLightSamples[shadow_idx] = float4(light_sample, 0.0f);
    }

    float2 s;
    s.x = SampleRandom(pixel_x, pixel_y, sample_idx, bounce, SAMPLE_TYPE_BXDF_U);
    s.y = SampleRandom(pixel_x, pixel_y, sample_idx, bounce, SAMPLE_TYPE_BXDF_V);
    float s1 = SampleRandom(pixel_x, pixel_y, sample_idx, bounce, SAMPLE_TYPE_BXDF_LAYER);
    float pdf;
    float offset;
    float3 outgoing_dir;
    float3 bxdf = SampleBxdf(s1, s, material, normal, incoming, outgoing_dir, pdf, offset);
    float3 new_throughput = pdf > 0.0f ? bxdf / pdf : 0.0f.xxx;
    g_Throughputs[pixel_idx] = float4(throughput * new_throughput, 0.0f);

    if (pdf > 0.0f)
    {
        uint outgoing_idx;
        InterlockedAdd(g_OutgoingRayCounter[0], 1, outgoing_idx);
        Ray outgoing_ray;
        outgoing_ray.origin = float4(position + geometry_normal * EPS * offset, 0.0f);
        outgoing_ray.direction = float4(outgoing_dir, MAX_RENDER_DIST);
        g_OutgoingRays[outgoing_idx] = outgoing_ray;
        g_OutgoingPixelIndices[outgoing_idx] = pixel_idx;
    }
}
