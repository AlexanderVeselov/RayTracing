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

#include "src/kernels/common/material.h"
#include "src/kernels/common/sampling.h"
#include "src/kernels/common/shared_structures.h"

float2 ProjectScreen(float3 position, KernelCamera camera)
{
    float3 camera_position = camera.position_fov.xyz;
    float3 camera_front = camera.front_aspect.xyz;
    float3 camera_up = camera.up_aperture.xyz;
    float3 d = normalize(position - camera_position);

    float3 ipd = d / dot(camera_front, d);
    float angle = tan(0.5f * camera.position_fov.w);

    float3 right = cross(camera_front, camera_up);
    float u = dot(right, ipd) / (angle * camera.front_aspect.w);
    float v = dot(camera_up, ipd) / (angle);

    return (float2)(u, v) * 0.5f + 0.5f;
}

__kernel void GenerateAOV(
    // Input
    __global Ray* rays, __global uint* ray_counter, __global uint* pixel_indices,
    __global Hit* hits, __global Vertex* vertices, __global uint* indices,
    __global uint* material_ids, __global PackedMaterial* materials, __global Texture* textures,
    __global uint* texture_data, uint width, uint height, KernelCamera camera,
    KernelCamera prev_camera,
    // Output
    __global float3* diffuse_albedo, __global float* depth_buffer, __global float3* normal_buffer,
    __global float2* velocity_buffer)
{
    uint ray_idx = get_global_id(0);
    uint num_rays = ray_counter[0];

    if (ray_idx >= num_rays)
    {
        return;
    }

    Hit hit = hits[ray_idx];

    if (hit.primitive_id == INVALID_ID)
    {
        return;
    }

    Ray ray = rays[ray_idx];
    float3 incoming = -ray.direction.xyz;

    uint pixel_idx = pixel_indices[ray_idx];

    int x = pixel_idx % width;
    int y = pixel_idx / width;

    uint index_offset = hit.primitive_id * 3;
    Vertex v1 = vertices[indices[index_offset + 0]];
    Vertex v2 = vertices[indices[index_offset + 1]];
    Vertex v3 = vertices[indices[index_offset + 2]];

    float3 position =
        InterpolateAttributes(v1.position.xyz, v2.position.xyz, v3.position.xyz, hit.bc);

    float3 geometry_normal =
        normalize(cross(v2.position.xyz - v1.position.xyz, v3.position.xyz - v1.position.xyz));

    float2 texcoord =
        InterpolateAttributes2(v1.texcoord.xy, v2.texcoord.xy, v3.texcoord.xy, hit.bc);

    float3 normal =
        normalize(InterpolateAttributes(v1.normal.xyz, v2.normal.xyz, v3.normal.xyz, hit.bc));

    PackedMaterial packed_material = materials[material_ids[hit.primitive_id]];
    Material material;
    ApplyTextures(packed_material, &material, texcoord, textures, texture_data);

    diffuse_albedo[pixel_idx] = material.diffuse_albedo;
    depth_buffer[pixel_idx] = length(ray.origin.xyz - position);
    normal_buffer[pixel_idx] = normal;
    velocity_buffer[pixel_idx] =
        ProjectScreen(position, camera) - ProjectScreen(position, prev_camera);
}
