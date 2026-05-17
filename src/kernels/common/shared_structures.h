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
#include <glm/glm.hpp>
#define float2 glm::vec2
#define float4 glm::vec4
#endif

#define MATERIAL_BLINN 1
#define MATERIAL_METAL 2
#define MATERIAL_ORENNAYAR 4
#define MATERIAL_PHONG 5

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_DIRECTIONAL 1

#ifdef GLSL
#define STRUCT_BEGIN(x) \
    struct x            \
    {
#define STRUCT_END(x) \
    }                 \
    ;
#else
#define STRUCT_BEGIN(x) \
    typedef struct x    \
    {
#define STRUCT_END(x) \
    }                 \
    x;
#endif

STRUCT_BEGIN(Ray)
float4 origin;     // w - t_min
float4 direction;  // w - t_max
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
unsigned int diffuse_albedo;       // 24 bit - RGB, 8 bit - texture index
unsigned int specular_albedo;      // 24 bit - RGB, 8 bit - texture index
unsigned int emission;             // 32 bit - RGBE
unsigned int roughness_metalness;  // 16 bit - roughness + texture idx, 16 bit - metalness + texture idx
unsigned int
    ior_emission_idx_transparency;  // 8 bit - ior, 8 bit - emission texture idx, 16 bit - transparency + texture idx
STRUCT_END(PackedMaterial)

STRUCT_BEGIN(Light)
float4 origin;
float4 radiance;
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
float4 position;
float2 texcoord;
unsigned int padding[2];
float4 normal;
STRUCT_END(Vertex)

STRUCT_BEGIN(RTTriangle)
float4 position1;
float4 position2;
float4 position3;
unsigned int prim_id;
unsigned int padding[3];
STRUCT_END(RTTriangle)

STRUCT_BEGIN(LinearBVHNode)
float4 bmin;
float4 bmax;
unsigned int offset;               // primitives (leaf) or second child (interior) offset
unsigned int num_primitives_axis;  // 0 -> interior node
unsigned int padding[2];
STRUCT_END(LinearBVHNode)

STRUCT_BEGIN(Camera)
float4 position_fov;  // x, y, z - position, w - fov
float4 front_aspect;  // x, y, z - front, w - aspect ratio
float4 up_aperture;   // x, y, z - up, w - aperture
float focus_distance;
unsigned int padding[3];
STRUCT_END(Camera)

#endif  // SHARED_STRUCTURES_H
