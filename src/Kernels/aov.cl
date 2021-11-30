/*****************************************************************************
 MIT License

 Copyright(c) 2021 Alexander Veselov

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

#include "shared_structures.h"
#include "material.h"
#include "sampling.h"
#include "light.h"

__kernel void GenerateAOV
(
    // Input
    __global Ray*            rays,
    __global uint*           ray_counter,
    __global uint*           pixel_indices,
    __global Hit*            hits,
    __global Triangle*       triangles,
    __global PackedMaterial* materials,
    __global Texture*        textures,
    __global uint*           texture_data,
    uint width,
    uint height,
    // Output
    __global float3* diffuse_albedo,
    __global float*  depth,
    __global float2* velocity
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

    Triangle triangle = triangles[hit.primitive_id];

    float3 position = InterpolateAttributes(triangle.v1.position,
        triangle.v2.position, triangle.v3.position, hit.bc);

    float3 geometry_normal = normalize(cross(triangle.v2.position - triangle.v1.position, triangle.v3.position - triangle.v1.position));

    float2 texcoord = InterpolateAttributes2(triangle.v1.texcoord.xy,
        triangle.v2.texcoord.xy, triangle.v3.texcoord.xy, hit.bc);

    float3 normal = normalize(InterpolateAttributes(triangle.v1.normal,
        triangle.v2.normal, triangle.v3.normal, hit.bc));

    PackedMaterial packed_material = materials[triangle.mtlIndex];
    Material material;
    ApplyTextures(packed_material, &material, texcoord, textures, texture_data);

    diffuse_albedo[pixel_idx] = material.diffuse_albedo;
    depth[pixel_idx] = length(ray.origin.xyz - position);
}
