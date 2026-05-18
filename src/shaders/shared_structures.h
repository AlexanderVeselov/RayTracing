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

#ifndef SHARED_STRUCTURES_H
#define SHARED_STRUCTURES_H

#ifdef __cplusplus
#include <algorithm>
#include <cstddef>
#include <glm/glm.hpp>
#define float2 glm::vec2
#define float3 glm::vec3
#define float4 glm::vec4
#endif

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_DIRECTIONAL 1
#define MAX_TEXTURES 255u

struct Ray
{
    float3 origin;
    float t_min;
    float3 direction;
    float t_max;
};

struct Hit
{
    float2 bc;
    unsigned int primitive_id;
    // TODO: remove t from hit structure
    float t;
};

struct SceneInfo
{
    unsigned int analytic_light_count;
    unsigned int emissive_count;
    unsigned int environment_map_index;
    unsigned int padding;
};

struct PackedMaterial
{
    unsigned int diffuse_albedo;       // 24 bit - RGB, 8 bit - texture index
    unsigned int specular_albedo;      // 24 bit - RGB, 8 bit - texture index
    unsigned int emission;             // 32 bit - RGBE
    unsigned int roughness_metalness;  // 16 bit - roughness + texture idx, 16 bit - metalness + texture idx
    unsigned int ior_emission_idx_transparency;  // 8 bit - ior, 8 bit - emission texture idx, 16 bit - transparency + texture idx
};

struct Light
{
    float3 origin;
    unsigned int type;
    float3 radiance;
    unsigned int padding;
};

struct Vertex
{
    float3 position;
    unsigned int padding1;
    float3 normal;
    unsigned int padding2;
    float2 texcoord;
    unsigned int padding3;
    unsigned int padding4;
};

struct RTTriangle
{
    float3 position1;
    unsigned int prim_id;
    float3 position2;
    unsigned int padding1;
    float3 position3;
    unsigned int padding2;
};

struct LinearBVHNode
{
    float3 bmin;
    unsigned int offset;               // primitives (leaf) or second child (interior) offset
    float3 bmax;
    unsigned int num_primitives_axis;  // 0 -> interior node
};

struct Camera
{
    float3 position;
    float fov;
    float3 front;
    float aspect_ratio;
    float3 up;
    float aperture;
    float focus_distance;
    unsigned int padding1;
    unsigned int padding2;
    unsigned int padding3;
};

#ifdef __cplusplus
static_assert(sizeof(Ray) == 32, "Ray layout must match HLSL");
static_assert(sizeof(Hit) == 16, "Hit layout must match HLSL");
static_assert(sizeof(SceneInfo) == 16, "SceneInfo layout must match HLSL");
static_assert(sizeof(PackedMaterial) == 20, "PackedMaterial layout must match HLSL");
static_assert(sizeof(Light) == 32, "Light layout must match HLSL");
static_assert(sizeof(Vertex) == 48, "Vertex layout must match HLSL");
static_assert(sizeof(RTTriangle) == 48, "RTTriangle layout must match HLSL");
static_assert(sizeof(LinearBVHNode) == 32, "LinearBVHNode layout must match HLSL");
static_assert(sizeof(Camera) == 64, "Camera layout must match HLSL");

static_assert(offsetof(Ray, origin) == 0, "Ray::origin offset must match HLSL");
static_assert(offsetof(Ray, t_min) == 12, "Ray::t_min offset must match HLSL");
static_assert(offsetof(Ray, direction) == 16, "Ray::direction offset must match HLSL");
static_assert(offsetof(Ray, t_max) == 28, "Ray::t_max offset must match HLSL");
static_assert(offsetof(Camera, position) == 0, "Camera::position offset must match HLSL");
static_assert(offsetof(Camera, fov) == 12, "Camera::fov offset must match HLSL");
static_assert(offsetof(Camera, front) == 16, "Camera::front offset must match HLSL");
static_assert(offsetof(Camera, aspect_ratio) == 28, "Camera::aspect_ratio offset must match HLSL");
static_assert(offsetof(Camera, up) == 32, "Camera::up offset must match HLSL");
static_assert(offsetof(Camera, aperture) == 44, "Camera::aperture offset must match HLSL");
static_assert(offsetof(Camera, focus_distance) == 48, "Camera::focus_distance offset must match HLSL");
#endif

#endif  // SHARED_STRUCTURES_H
