#include "frame_data.hlsli"
#include "material.hlsli"

RWStructuredBuffer<Ray> g_IncomingRays : register(u2);
RWStructuredBuffer<Ray> g_OutgoingRays : register(u3);
RWStructuredBuffer<uint> g_IncomingRayCounter : register(u4);
RWStructuredBuffer<uint> g_IncomingPixelIndices : register(u5);
RWStructuredBuffer<uint> g_OutgoingPixelIndices : register(u6);
RWStructuredBuffer<Hit> g_Hits : register(u7);
RWStructuredBuffer<Ray> g_ShadowRays : register(u8);
RWStructuredBuffer<uint> g_ShadowRayCounter : register(u9);
RWStructuredBuffer<uint> g_ShadowPixelIndices : register(u10);
RWStructuredBuffer<float4> g_Throughputs : register(u12);
RWStructuredBuffer<float4> g_Radiance : register(u13);
RWStructuredBuffer<float4> g_DirectLightSamples : register(u14);
RWStructuredBuffer<uint> g_SampleCounter : register(u15);
RWStructuredBuffer<uint> g_OutgoingRayCounter : register(u19);
StructuredBuffer<RhiTriangle> g_Triangles : register(t20);
StructuredBuffer<PackedMaterial> g_Materials : register(t22);
StructuredBuffer<Light> g_Lights : register(t23);

float3 SampleFirstLight(
    float3 position, out float3 outgoing, out float pdf, out float distance_to_light)
{
    if (g_SceneCounts.z == 0)
    {
        outgoing = 0.0f.xxx;
        pdf = 0.0f;
        distance_to_light = 0.0f;
        return 0.0f.xxx;
    }

    Light light = g_Lights[0];
    pdf = 1.0f;

    if (light.type == LIGHT_TYPE_POINT)
    {
        outgoing = light.origin.xyz - position;
        float sq_length = max(dot(outgoing, outgoing), EPS);
        distance_to_light = sqrt(sq_length);
        return light.radiance.xyz / sq_length;
    }

    outgoing = normalize(light.origin.xyz);
    distance_to_light = MAX_RENDER_DIST;
    return light.radiance.xyz;
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
    g_Radiance[pixel_idx].xyz += throughput * material.emission;

    float3 light_outgoing;
    float light_pdf;
    float distance_to_light;
    float3 light_radiance =
        SampleFirstLight(position, light_outgoing, light_pdf, distance_to_light);
    if (light_pdf > 0.0f && dot(light_radiance, light_radiance) > 0.0f)
    {
        float3 outgoing = normalize(light_outgoing);
        float3 brdf = EvaluateMaterial(material, normal, incoming, outgoing);
        float3 light_sample =
            light_radiance * throughput * brdf * max(dot(outgoing, normal), 0.0f) / light_pdf;

        if (dot(light_sample, light_sample) > 0.0f)
        {
            uint shadow_idx;
            InterlockedAdd(g_ShadowRayCounter[0], 1, shadow_idx);
            Ray shadow_ray;
            shadow_ray.origin = float4(position + geometry_normal * EPS, 0.0f);
            shadow_ray.direction = float4(outgoing, distance_to_light - EPS);
            g_ShadowRays[shadow_idx] = shadow_ray;
            g_ShadowPixelIndices[shadow_idx] = pixel_idx;
            g_DirectLightSamples[shadow_idx] = float4(light_sample, 0.0f);
        }
    }

    uint seed = WangHash(pixel_idx + WangHash(g_SampleCounter[0]));
    float2 s = float2(RandomFloat(seed), RandomFloat(seed));
    float pdf;
    float3 tbn_outgoing = SampleHemisphereCosine(s, pdf);
    float3 outgoing_dir = TangentToWorld(tbn_outgoing, normal);
    float3 bxdf = material.diffuse_albedo * INV_PI * max(dot(outgoing_dir, normal), 0.0f);
    float3 new_throughput = pdf > 0.0f ? bxdf / pdf : 0.0f.xxx;
    g_Throughputs[pixel_idx] = float4(throughput * new_throughput, 0.0f);

    if (pdf > 0.0f && dot(new_throughput, new_throughput) > 0.0f)
    {
        uint outgoing_idx;
        InterlockedAdd(g_OutgoingRayCounter[0], 1, outgoing_idx);
        Ray outgoing_ray;
        outgoing_ray.origin = float4(position + geometry_normal * EPS, 0.0f);
        outgoing_ray.direction = float4(outgoing_dir, MAX_RENDER_DIST);
        g_OutgoingRays[outgoing_idx] = outgoing_ray;
        g_OutgoingPixelIndices[outgoing_idx] = pixel_idx;
    }
}
