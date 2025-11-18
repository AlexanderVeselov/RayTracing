/*****************************************************************************
 MIT License

 Copyright(c) 2023 Alexander Veselov

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

#include "src/kernels/common/shared_structures.h"
#include "src/kernels/common/material.h"
#include "src/kernels/common/sampling.h"
#include "src/kernels/common/light.h"

__kernel void HitSurface
(
    // Input
    __global Ray*            incoming_rays,
    __global uint*           incoming_ray_counter,
    __global uint*           incoming_pixel_indices,
    __global Hit*            hits,
    __global Vertex*         vertices,
    __global uint*           indices,
    __global uint*           material_ids,
    __global Light*          analytic_lights,
    __global uint*           emissive_indices,
    __global PackedMaterial* materials,
    __global Texture*        textures,
    __global uint*           texture_data,
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

    const uint prim = hit.primitive_id;
    const uint i0 = indices[prim * 3u + 0u];
    const uint i1 = indices[prim * 3u + 1u];
    const uint i2 = indices[prim * 3u + 2u];

    const Vertex v0 = vertices[i0];
    const Vertex v1 = vertices[i1];
    const Vertex v2 = vertices[i2];

    // Compute barycentric coordinates
    float2 bc = CalcBarycentrics(incoming_ray, v0.position, v1.position, v2.position);

    // Interpolated hit position
    float3 position = InterpolateAttributes(v0.position, v1.position, v2.position, bc);

    // Geometric (non-interpolated) normal
    float3 geometry_normal = normalize(cross(v1.position - v0.position, v2.position - v0.position));

    // Interpolated texcoord and shading normal (decode packed normals first)
    const float2 texcoord = InterpolateAttributes2(v0.texcoord, v1.texcoord, v2.texcoord, bc);
    float3 n0 = DecodeOctNormal(v0.normal);
    float3 n1 = DecodeOctNormal(v1.normal);
    float3 n2 = DecodeOctNormal(v2.normal);
    const float3 normal = normalize(InterpolateAttributes(n0, n1, n2, bc));

    // Get packed material
    const uint material_id = material_ids[prim];
    const PackedMaterial packed_material = materials[material_id];

    // Unpack material
    Material material;
    ApplyTextures(packed_material, &material, texcoord, textures, texture_data);

    float3 hit_throughput = throughputs[pixel_idx];

#ifndef ENABLE_WHITE_FURNACE
    if (dot(material.emission.xyz, (float3)(1.0f, 1.0f, 1.0f)) > 0.0f)
    {
        result_radiance[pixel_idx].xyz += hit_throughput * material.emission.xyz;
    }
#endif // ENABLE_WHITE_FURNACE

    // Direct lighting
    {
        float s_light = SampleRandom(x, y, sample_idx, bounce, SAMPLE_TYPE_LIGHT, BLUE_NOISE_BUFFERS);
        float3 outgoing;
        float pdf;
        float3 light_radiance = Light_Sample(analytic_lights, scene_info, position, normal, s_light, &outgoing, &pdf);

        float distance_to_light = length(outgoing);
        outgoing = normalize(outgoing);

        float3 brdf = EvaluateMaterial(material, normal, incoming, outgoing);
        float3 light_sample = light_radiance * hit_throughput * brdf / pdf * max(dot(outgoing, normal), 0.0f);

        bool spawn_shadow_ray = (pdf > 0.0f) && (dot(light_sample, light_sample) > 0.0f);

        if (spawn_shadow_ray)
        {
            Ray shadow_ray;
            shadow_ray.origin.xyz = position + normal * EPS;
            shadow_ray.origin.w = 0.0f;
            shadow_ray.direction.xyz = outgoing;
            shadow_ray.direction.w = distance_to_light;

            ///@TODO: use LDS
            uint shadow_ray_idx = atomic_add(shadow_ray_counter, 1);

            // Store to the memory
            shadow_rays[shadow_ray_idx] = shadow_ray;
            shadow_pixel_indices[shadow_ray_idx] = pixel_idx;
            direct_light_samples[shadow_ray_idx] = light_sample;
        }
    }

    // Indirect lighting
    {
        // Sample bxdf
        float2 s;
        s.x = SampleRandom(x, y, sample_idx, bounce, SAMPLE_TYPE_BXDF_U, BLUE_NOISE_BUFFERS);
        s.y = SampleRandom(x, y, sample_idx, bounce, SAMPLE_TYPE_BXDF_V, BLUE_NOISE_BUFFERS);
        float s1 = SampleRandom(x, y, sample_idx, bounce, SAMPLE_TYPE_BXDF_LAYER, BLUE_NOISE_BUFFERS);

        float pdf = 0.0f;
        float3 throughput = 0.0f;
        float3 outgoing;
        float offset;
        float3 bxdf = SampleBxdf(s1, s, material, normal, incoming, &outgoing, &pdf, &offset);

        if (pdf > 0.0)
        {
            throughput = bxdf / pdf;
        }

        throughputs[pixel_idx] *= throughput;

        bool spawn_outgoing_ray = (pdf > 0.0);

        if (spawn_outgoing_ray)
        {
            ///@TODO: use LDS
            uint outgoing_ray_idx = atomic_add(outgoing_ray_counter, 1);

            Ray outgoing_ray;
            outgoing_ray.origin.xyz = position + geometry_normal * EPS * offset;
            outgoing_ray.origin.w = 0.0f;
            outgoing_ray.direction.xyz = outgoing;
            outgoing_ray.direction.w = MAX_RENDER_DIST;

            outgoing_rays[outgoing_ray_idx] = outgoing_ray;
            outgoing_pixel_indices[outgoing_ray_idx] = pixel_idx;
        }
    }

}
