#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#ifdef __cplusplus
#include "mathlib/mathlib.hpp"
#include <CL/cl.h>
#include <algorithm>
#endif

#define MATERIAL_BLINN 1
#define MATERIAL_METAL 2
#define MATERIAL_ORENNAYAR 4
#define MATERIAL_PHONG 5

#ifndef __cplusplus
typedef struct
{
    float3 pos[2];
} Bounds3;
#endif

typedef struct Material
{
#ifdef __cplusplus
    Material() {}
#endif
    float3 diffuse;
    float3 specular;
    float3 emission;
    unsigned int type;
    float roughness;
    float ior;
    int padding;

} Material;

typedef struct Vertex
{
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
    float3 tangent_s;

} Vertex;

typedef struct Triangle
{
#ifdef __cplusplus
    Triangle(Vertex v1, Vertex v2, Vertex v3, unsigned int mtlIndex)
        : v1(v1), v2(v2), v3(v3), mtlIndex(mtlIndex)
    {}

    void Project(float3 axis, float &min, float &max) const
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

} Triangle;

typedef struct CellData
{
    unsigned int start_index;
    unsigned int count;
} CellData;


typedef struct LinearBVHNode
{
#ifdef __cplusplus
    LinearBVHNode() {}
#endif
    // 32 bytes
    Bounds3 bounds;
    // 4 bytes
    unsigned int offset; // primitives (leaf) or second child (interior) offset
    // 2 bytes
    unsigned short nPrimitives;  // 0 -> interior node
    // 1 byte
    unsigned char axis;          // interior node: xyz
    unsigned char pad[9];        // ensure 48 byte total size

} LinearBVHNode;

#endif // TRIANGLE_HPP
