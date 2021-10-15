#include "shared_structures.h"
#include "bxdf.h"
#include "utils.h"
#include "sampling.h"

float3 SampleDiffuse(float2 s, float3 albedo, float3 f0, float3 normal,
    float3 incoming, float3* outgoing, float* pdf)
{
    float3 tbn_outgoing = SampleHemisphereCosine(s, pdf);
    *outgoing = TangentToWorld(tbn_outgoing, normal);

    return albedo * INV_PI;
}

float3 SampleSpecular(float2 s, float3 f0, float alpha,
    float3 normal, float3 incoming, float3* outgoing, float* pdf)
{
    if (alpha <= 1e-4f)
    {
        *outgoing = reflect(incoming, normal);
        *pdf = 1.0;
        float n_dot_o = dot(*outgoing, normal);
        // Don't apply fresnel here, it's applied in the external function
        return 1.0f / n_dot_o;
    }
    else
    {
        float3 wh = GGX_Sample(s, normal, alpha);
        *outgoing = reflect(incoming, wh);

        float n_dot_o = dot(normal, *outgoing);
        float n_dot_h = dot(normal, wh);
        float n_dot_i = dot(normal, incoming);
        float h_dot_o = dot(*outgoing, wh);

        float D = GGX_D(alpha, n_dot_h);
        // Don't apply fresnel here, it's applied in the external function
        //float3 F = FresnelSchlick(f0, h_dot_o);
        float G = V_SmithGGXCorrelated(n_dot_i, n_dot_o, alpha);

        *pdf = D * n_dot_h / (4.0f * dot(wh, *outgoing));

        return D /* F */ * G;
    }
}

float3 EvaluateSpecular(float alpha, float n_dot_i, float n_dot_o, float n_dot_h)
{
    float D = GGX_D(alpha, n_dot_h);
    float G = V_SmithGGXCorrelated(n_dot_i, n_dot_o, alpha);

    return D * G;
}

float3 EvaluateDiffuse(float3 albedo)
{
    return albedo * Lambert();
}

float3 EvaluateMaterial(Material material, float3 normal, float3 incoming, float3 outgoing)
{
    float3 half_vec = normalize(incoming + outgoing);
    float n_dot_i = max(dot(normal, incoming), EPS);
    float n_dot_o = max(dot(normal, outgoing), EPS);
    float n_dot_h = max(dot(normal, half_vec), EPS);
    float h_dot_o = max(dot(half_vec, outgoing), EPS);

    // Perceptual roughness remapping
    // Perceptual roughness remapping
    float roughness = material.roughness;
    float alpha = roughness * roughness;

    // Compute f0 values for metals and dielectrics
    float3 f0_dielectric = IorToF0(1.0f, material.ior);
    float3 f0_metal = material.specular_albedo.xyz;

    // Blend f0 values based on metalness
    float3 f0 = mix(f0_dielectric, f0_metal, material.metalness);

    // Since metals don't have the diffuse term, fade it to zero
    //@TODO: precompute it?
    float3 diffuse_color = (1.0f - material.metalness) * material.diffuse_albedo.xyz;

    // This is the scaling value for specular bxdf
    float3 specular_albedo = mix(material.specular_albedo.xyz, 1.0, material.metalness);

    float3 fresnel = FresnelSchlick(f0, h_dot_o);

    float3 specular = EvaluateSpecular(alpha, n_dot_i, n_dot_o, n_dot_h);
    float3 diffuse = EvaluateDiffuse(diffuse_color);

    return fresnel * specular + (1.0f - fresnel) * diffuse;
}

float3 SampleBxdf(float s1, float2 s, Material material, float3 normal,
    float3 incoming, float3* outgoing, float* pdf)
{
#ifdef ENABLE_WHITE_FURNACE
    material.diffuse_albedo = 1.0f;
    material.specular_albedo = 1.0f;
#endif // ENABLE_WHITE_FURNACE

    // Perceptual roughness remapping
    float roughness = material.roughness;
    float alpha = roughness * roughness;

    // Compute f0 values for metals and dielectrics
    float3 f0_dielectric = IorToF0(1.0f, material.ior);
    float3 f0_metal = material.specular_albedo.xyz;

    // Blend f0 values based on metalness
    float3 f0 = mix(f0_dielectric, f0_metal, material.metalness);

    // Since metals don't have the diffuse term, fade it to zero
    //@TODO: precompute it?
    float3 diffuse_albedo = (1.0f - material.metalness) * material.diffuse_albedo.xyz;

    // This is the scaling value for specular bxdf
    float3 specular_albedo = mix(material.specular_albedo.xyz, 1.0, material.metalness);

    // This is not an actual fresnel value, because we need to use half vector instead of normal here
    // it's a "heuristic" used for better layer importance sampling and energy conservation
    float3 fresnel = FresnelSchlick(f0, dot(normal, incoming)) * specular_albedo;

    float specular_weight = Luma(specular_albedo * fresnel);
    float diffuse_weight = Luma(diffuse_albedo * (1.0f - fresnel));
    float weight_sum = diffuse_weight + specular_weight;

    float specular_sampling_pdf = specular_weight / weight_sum;
    float diffuse_sampling_pdf = diffuse_weight / weight_sum;

    //@TODO: reduce diffuse intensity where the specular value is high
    //@TODO: evaluate all layers at once?

    float3 bxdf = 0.0f;

    if (s1 <= specular_sampling_pdf)
    {
        // Sample specular
        bxdf = fresnel * SampleSpecular(s, f0, alpha, normal, incoming, outgoing, pdf);
        *pdf *= specular_sampling_pdf;
    }
    else
    {
        // Sample diffuse
        bxdf = (1.0f - fresnel) * SampleDiffuse(s, diffuse_albedo, f0, normal, incoming, outgoing, pdf);
        *pdf *= diffuse_sampling_pdf;
    }

    return bxdf;
}

__kernel void HitSurface
(
    // Input
    __global Ray*      incoming_rays,
    __global uint*     incoming_ray_counter,
    __global uint*     incoming_pixel_indices,
    __global Hit*      hits,
    __global Triangle* triangles,
    __global Light*    analytic_lights,
    __global uint*     emissive_indices,
    __global Material* materials,
    uint bounce,
    uint width,
    uint height,
    __global uint* sample_counter,
    SceneInfo scene_info,
    // Blue noise sampler
    __global int* sobol_256spp_256d,
    __global int* scramblingTile,
    __global int* rankingTile,
    // Output
    __global float3* throughputs,
    __global Ray*    outgoing_rays,
    __global uint*   outgoing_ray_counter,
    __global uint*   outgoing_pixel_indices,
    __global Ray*    shadow_rays,
    __global uint*   shadow_ray_counter,
    __global uint*   shadow_pixel_indices,
    __global float3* direct_light_samples,
    __global float4* result_radiance
)
{
    uint incoming_ray_idx = get_global_id(0);
    uint num_incoming_rays = incoming_ray_counter[0];

    if (incoming_ray_idx >= num_incoming_rays)
    {
        return;
    }

    Hit hit = hits[incoming_ray_idx];

    if (hit.primitive_id == INVALID_ID)
    {
        return;
    }

    Ray incoming_ray = incoming_rays[incoming_ray_idx];
    float3 incoming = -incoming_ray.direction.xyz;

    uint pixel_idx = incoming_pixel_indices[incoming_ray_idx];
    uint sample_idx = sample_counter[0];

    int x = pixel_idx % width;
    int y = pixel_idx / width;

    Triangle triangle = triangles[hit.primitive_id];

    float3 position = InterpolateAttributes(triangle.v1.position,
        triangle.v2.position, triangle.v3.position, hit.bc);

    float2 texcoord = InterpolateAttributes2(triangle.v1.texcoord.xy,
        triangle.v2.texcoord.xy, triangle.v3.texcoord.xy, hit.bc);

    float3 normal = normalize(InterpolateAttributes(triangle.v1.normal,
        triangle.v2.normal, triangle.v3.normal, hit.bc));

    Material material = materials[triangle.mtlIndex];

    float3 hit_throughput = throughputs[pixel_idx];

#ifndef ENABLE_WHITE_FURNACE
    if (dot(material.emission, (float3)(1.0f, 1.0f, 1.0f)) > 0.0f)
    {
        result_radiance[pixel_idx].xyz += hit_throughput * material.emission;
    }
#endif // ENABLE_WHITE_FURNACE

    bool spawn_shadow_ray = true;
    if (spawn_shadow_ray)
    {
        Ray shadow_ray;
        shadow_ray.origin.xyz = position + normal * EPS;
        shadow_ray.origin.w = 0.0f;
        shadow_ray.direction.xyz = normalize((float3)(-1.0f, -1.0f, 1.0f));
        shadow_ray.direction.w = MAX_RENDER_DIST;

        float3 brdf = EvaluateMaterial(material, normal, incoming, shadow_ray.direction.xyz);
        float3 light_radiance = (float3)(1.0f, 1.0f, 1.0f) * 10.0f;
        float3 light_sample = light_radiance * hit_throughput * max(brdf, (float3)(0.0f, 0.0f, 0.0f)) * max(dot(shadow_ray.direction.xyz, normal), 0.0f);

        ///@TODO: use LDS
        uint shadow_ray_idx = atomic_add(shadow_ray_counter, 1);

        // Store to the memory
        shadow_rays[shadow_ray_idx] = shadow_ray;
        shadow_pixel_indices[shadow_ray_idx] = pixel_idx;
        direct_light_samples[shadow_ray_idx] = light_sample;
    }

    // Sample bxdf
    float2 s;
    s.x = SampleRandom(x, y, sample_idx, bounce, SAMPLE_TYPE_BXDF_U, BLUE_NOISE_BUFFERS);
    s.y = SampleRandom(x, y, sample_idx, bounce, SAMPLE_TYPE_BXDF_V, BLUE_NOISE_BUFFERS);
    float s1 = SampleRandom(x, y, sample_idx, bounce, SAMPLE_TYPE_BXDF_LAYER, BLUE_NOISE_BUFFERS);

    float pdf = 0.0f;
    float3 throughput = 0.0f;
    float3 outgoing;
    float3 bxdf = SampleBxdf(s1, s, material, normal, incoming, &outgoing, &pdf);

    if (pdf > 0.0)
    {
        throughput = bxdf / pdf * max(dot(outgoing, normal), 0.0f);
    }

    throughputs[pixel_idx] *= throughput;

    bool spawn_outgoing_ray = (pdf > 0.0);

    if (spawn_outgoing_ray)
    {
        ///@TODO: use LDS
        uint outgoing_ray_idx = atomic_add(outgoing_ray_counter, 1);

        Ray outgoing_ray;
        outgoing_ray.origin.xyz = position + normal * EPS;
        outgoing_ray.origin.w = 0.0f;
        outgoing_ray.direction.xyz = outgoing;
        outgoing_ray.direction.w = MAX_RENDER_DIST;

        outgoing_rays[outgoing_ray_idx] = outgoing_ray;
        outgoing_pixel_indices[outgoing_ray_idx] = pixel_idx;
    }

}
