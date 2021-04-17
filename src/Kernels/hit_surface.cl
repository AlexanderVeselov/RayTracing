#include "shared_structures.h"
#include "bxdf.h"
#include "utils.h"

float SampleBlueNoise(int pixel_i, int pixel_j, int sampleIndex, int sampleDimension,
    __global int* sobol_256spp_256d, __global int* scramblingTile, __global int* rankingTile)
{
    // wrap arguments
    pixel_i = pixel_i & 127;
    pixel_j = pixel_j & 127;
    sampleIndex = sampleIndex & 255;
    sampleDimension = sampleDimension & 255;

    // xor index based on optimized ranking
    int rankedSampleIndex = sampleIndex ^ rankingTile[sampleDimension + (pixel_i + pixel_j * 128) * 8];

    // fetch value in sequence
    int value = sobol_256spp_256d[sampleDimension + rankedSampleIndex * 256];

    // If the dimension is optimized, xor sequence value based on optimized scrambling
    value = value ^ scramblingTile[(sampleDimension % 8) + (pixel_i + pixel_j * 128) * 8];

    // convert to float and return
    float v = (0.5f + value) / 256.0f;
    return v;
}

float3 SampleDiffuse(float2 s, float3 albedo, float3 normal, float3* outgoing, float* pdf)
{
    float3 tbn_outgoing = SampleHemisphereCosine(s, pdf);
    *outgoing = TangentToWorld(tbn_outgoing, normal);

    return albedo * INV_PI;
}

float3 SampleSpecular(float2 s, float3 f0, float alpha, float3 normal, float3 incoming, float3* outgoing, float* pdf)
{
    if (alpha <= 1e-4f)
    {
        *outgoing = reflect(incoming, normal);
        *pdf = 1.0;
        float n_dot_o = dot(*outgoing, normal);
        return FresnelSchlick(f0, n_dot_o) / n_dot_o;
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
        float3 F = FresnelSchlick(f0, h_dot_o);
        float G = V_SmithGGXCorrelated(n_dot_i, n_dot_o, alpha);

        *pdf = D * n_dot_h / (4.0f * dot(wh, *outgoing));

        return D * F * G;
    }
}

float3 SampleBxdf(float s1, float2 s, Material material, float3 normal,
    float3 incoming, float3* outgoing, float* pdf)
{
    // Perceptual roughness remapping
    float roughness = material.roughness;
    float alpha = roughness * roughness;

    // Frostbite remapping function for dielectrics
    // https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
    float3 f0_dielectric = 0.16f * material.reflectance * material.reflectance;
    float3 f0_metal = material.albedo.xyz;

    // TODO: lerp
    float3 f0 = f0_dielectric * (1.0f - material.metalness) + f0_metal * material.metalness;
    //float3 fresnel = FresnelSchlick(f0, h_dot_o);
    // Since metals don't have diffuse term, fade it to zero
    float3 diffuse_color = (1.0f - material.metalness) * material.albedo.xyz;

    float specular_probability = material.metalness;
    float diffuse_probability = 1.0f - specular_probability;

    float3 bxdf = 0.0f;

    if (s1 <= specular_probability)
    {
        // Sample specular
        bxdf = SampleSpecular(s, f0, alpha, normal, incoming, outgoing, pdf);
        *pdf *= specular_probability;
    }
    else
    {
        // Sample diffuse
        bxdf = SampleDiffuse(s, diffuse_color, normal, outgoing, pdf);
        *pdf *= diffuse_probability;
    }

    return bxdf;
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
    float n_dot_i = dot(normal, incoming);
    float n_dot_o = dot(normal, outgoing);
    float n_dot_h = dot(normal, half_vec);
    float h_dot_o = dot(half_vec, outgoing);

    // Perceptual roughness remapping
    float alpha = material.roughness * material.roughness;

    // Frostbite remapping function for dielectrics
    // https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
    float3 f0_dielectric = 0.16f * material.reflectance * material.reflectance;
    float3 f0_metal = material.albedo.xyz;

    // TODO: lerp
    float3 f0 = f0_dielectric * (1.0f - material.metalness) + f0_metal * material.metalness;

    // Since metals don't have diffuse term, fade it to zero
    float3 diffuse_color = (1.0f - material.metalness) * material.albedo.xyz;

    float3 fresnel = FresnelSchlick(f0, h_dot_o);

    float3 specular = EvaluateSpecular(alpha, n_dot_i, n_dot_o, n_dot_h);
    float3 diffuse = EvaluateDiffuse(diffuse_color);

    return fresnel * specular + (1.0f - fresnel) * diffuse;

}

__kernel void KernelEntry
(
    // Input
    __global Ray* incoming_rays,
    __global uint* incoming_ray_counter,
    __global uint* incoming_pixel_indices,
    __global Hit* hits,
    __global Triangle* triangles,
    __global Material* materials,
    uint bounce,
    uint width,
    uint height,
    __global uint* sample_counter,
    // Sampler
    __global int* sobol_256spp_256d,
    __global int* scramblingTile,
    __global int* rankingTile,
    // Output
    __global float3* throughputs,
    __global Ray* outgoing_rays,
    __global uint* outgoing_ray_counter,
    __global uint* outgoing_pixel_indices,
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

    if (dot(material.emission, (float3)(1.0f, 1.0f, 1.0f)) > 0.0f)
    {
        result_radiance[pixel_idx].xyz += hit_throughput * material.emission;
    }

    // Sample bxdf
    float2 s;
    s.x = SampleBlueNoise(x, y, sample_idx, bounce * 3 + 0, sobol_256spp_256d, scramblingTile, rankingTile);
    s.y = SampleBlueNoise(x, y, sample_idx, bounce * 3 + 1, sobol_256spp_256d, scramblingTile, rankingTile);
    float s1 = SampleBlueNoise(x, y, sample_idx, bounce * 3 + 2, sobol_256spp_256d, scramblingTile, rankingTile);

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
        ///@TODO: reduct atomic memory traffic by using LDS
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
