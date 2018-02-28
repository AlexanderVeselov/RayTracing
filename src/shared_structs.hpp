#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include "mathlib.hpp"
#include <CL/cl.h>
#include <algorithm>

struct Material
{
    Material() : diffuse(1.0), specular(0.0), emission(0.0)
    {
    }
    Material(float3 diff, float3 spec, float3 emiss) :
        diffuse(diff), specular(spec), emission(emiss)
    {
    }

    float3 diffuse;
    float3 specular;
    float3 emission;

};

struct Vertex
{
    Vertex() {}
    Vertex(const float3& position, const float2& texcoord, const float3& normal)
        : position(position), texcoord(texcoord.x, texcoord.y, 0), 
        normal(normal)
    {}

    float3 position;
    float3 texcoord;
    float3 normal;
    float3 tangent_s;

};

struct Triangle
{
    Triangle(Vertex v1, Vertex v2, Vertex v3)
        : v1(v1), v2(v2), v3(v3)
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

    Vertex v1, v2, v3;
    // Pointers aren't allowed to us, deal with array indices
    //unsigned int mtlIndex;

};

struct CellData
{
    cl_uint start_index;
    cl_uint count;

};


struct LinearBVHNode
{
    LinearBVHNode() {}
    // 32 bytes
    Bounds3 bounds;
    // 4 bytes
    cl_int offset; // primitives (leaf) or second child (interior) offset

    // 2 bytes
    cl_ushort nPrimitives;  // 0 -> interior node
    // 1 byte
    cl_uchar axis;          // interior node: xyz
    cl_uchar pad[9];        // ensure 48 byte total size

};

#endif // TRIANGLE_HPP
