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

float2 ProjectScreen(float3 position, Camera camera)
{
    float3 d = normalize(position - camera.position);

    float3 ipd = d / dot(camera.front, d);
    float angle = tan(0.5f * camera.fov);

    float3 right = cross(camera.front, camera.up);
    float u = dot(right, ipd) / (angle * camera.aspect_ratio);
    float v = dot(camera.up, ipd) / (angle);

    return (float2)(u, v) * 0.5f + 0.5f;
}

__kernel void GenerateAOV
(
    // Input
    __global Ray*            rays,
    __global uint*           ray_counter,
    __global uint*           pixel_indices,
    __global Hit*            hits,
    __global Vertex*         vertices,
    __global uint*           indices,
    __global uint*           material_ids,
    __global PackedMaterial* materials,
    __global Texture*        textures,
    __global uint*           texture_data,
    uint width,
    uint height,
    Camera camera,
    Camera prev_camera,
    // Output
    __global float3* diffuse_albedo,
    __global float*  depth_buffer,
    __global float3* normal_buffer,
    __global float2* velocity_buffer
)
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

    const uint prim = hit.primitive_id;
    const uint i0 = indices[prim * 3u + 0u];
    const uint i1 = indices[prim * 3u + 1u];
    const uint i2 = indices[prim * 3u + 2u];

    const Vertex v0 = vertices[i0];
    const Vertex v1 = vertices[i1];
    const Vertex v2 = vertices[i2];

    // Compute barycentric coordinates
    float2 bc = CalcBarycentrics(ray, v0.position, v1.position, v2.position);

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

    diffuse_albedo[pixel_idx] = material.diffuse_albedo;
    depth_buffer[pixel_idx] = length(ray.origin.xyz - position);
    normal_buffer[pixel_idx] = normal;
    velocity_buffer[pixel_idx] = ProjectScreen(position, camera) - ProjectScreen(position, prev_camera);
}
