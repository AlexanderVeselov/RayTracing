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

#ifndef SHARED_STRUCTURES_HPP
#define SHARED_STRUCTURES_HPP

#ifdef __cplusplus
#include "mathlib/mathlib.hpp"
#include <CL/cl.h>
#include <algorithm>
#endif

#define MATERIAL_BLINN 1
#define MATERIAL_METAL 2
#define MATERIAL_ORENNAYAR 4
#define MATERIAL_PHONG 5

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_DIRECTIONAL 1

#ifdef GLSL
#define STRUCT_BEGIN(x) struct x {
#define STRUCT_END(x) };
#else
#define STRUCT_BEGIN(x) typedef struct x {
#define STRUCT_END(x) } x;
#endif

#ifndef __cplusplus
STRUCT_BEGIN(Bounds3)
    float3 pos[2];
STRUCT_END(Bounds3)
#endif

STRUCT_BEGIN(Ray)
    float4 origin; // w - t_min
    float4 direction; // w - t_max
STRUCT_END(Ray)

STRUCT_BEGIN(Hit)
    float2 bc;
    unsigned int primitive_id;
    // TODO: remove t from hit structure
    float t;
STRUCT_END(Hit)

STRUCT_BEGIN(SceneInfo)
    unsigned int analytic_light_count;
    unsigned int emissive_count;
    unsigned int environment_map_index;
    unsigned int padding;
STRUCT_END(SceneInfo)

STRUCT_BEGIN(PackedMaterial)
    unsigned int diffuse_albedo;                // 24 bit - RGB, 8 bit - texture index
    unsigned int specular_albedo;               // 24 bit - RGB, 8 bit - texture index
    unsigned int emission;                      // 32 bit - RGBE
    unsigned int roughness_metalness;           // 16 bit - roughness + texture idx, 16 bit - metalness + texture idx
    unsigned int ior_emission_idx_transparency; // 8 bit - ior, 8 bit - emission texture idx, 16 bit - transparency + texture idx
STRUCT_END(PackedMaterial)

STRUCT_BEGIN(Light)
    float3 origin;
    float3 radiance;
    unsigned int type;
    unsigned int padding[3];
STRUCT_END(Light)

STRUCT_BEGIN(Texture)
    int data_start;
    int width;
    int height;
    int padding;
STRUCT_END(Texture)

STRUCT_BEGIN(Vertex)
#ifdef __cplusplus
    Vertex() {}
    Vertex(const float3& position, const float2& texcoord, const float3& normal)
        : position(position), texcoord(texcoord.x, texcoord.y, 0), 
        normal(normal)
    {}
#endif

    float3 position;
    float3 texcoord;
    float3 normal;
STRUCT_END(Vertex)

STRUCT_BEGIN(Triangle)
#ifdef __cplusplus
    Triangle(Vertex v1, Vertex v2, Vertex v3, unsigned int mtlIndex)
        : v1(v1), v2(v2), v3(v3), mtlIndex(mtlIndex)
    {}

    void Project(float3 axis, float& min, float& max) const
    {
        min = std::numeric_limits<float>::max();
        max = std::numeric_limits<float>::lowest();

        float3 points[3] = { v1.position, v2.position, v3.position };

        for (size_t i = 0; i < 3; ++i)
        {
            float val = Dot(points[i], axis);
            min = std::min(min, val);
            max = std::max(max, val);
        }
    }

    Bounds3 GetBounds() const
    {
        return Union(Bounds3(v1.position, v2.position), v3.position);
    }
#endif

    Vertex v1, v2, v3;
    unsigned int mtlIndex;
    unsigned int padding[3];
STRUCT_END(Triangle)

STRUCT_BEGIN(RTTriangle)
#ifdef __cplusplus
    RTTriangle(float3 v1, float3 v2, float3 v3)
        : position1(v1), position2(v2), position3(v3)
    {}
#endif

    float3 position1;
    float3 position2;
    float3 position3;
STRUCT_END(RTTriangle)

STRUCT_BEGIN(CellData)
    unsigned int start_index;
    unsigned int count;
STRUCT_END(CellData)

STRUCT_BEGIN(LinearBVHNode)
#ifdef __cplusplus
    LinearBVHNode() {}
#endif
    // 32 bytes
    Bounds3 bounds;
    // 4 bytes
    unsigned int offset; // primitives (leaf) or second child (interior) offset
    // 4 bytes
    unsigned int num_primitives_axis;  // 0 -> interior node
    unsigned int padding[2]; // ensure 48 byte total size
STRUCT_END(LinearBVHNode)

STRUCT_BEGIN(Camera)
    float3 position;
    float3 front;
    float3 up;
    float  fov;
    float  aspect_ratio;
    float  aperture;
    float  focus_distance;
STRUCT_END(Camera)

#endif // SHARED_STRUCTURES_HPP
