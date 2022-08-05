/*****************************************************************************
 MIT License

 Copyright(c) 2022 Alexander Veselov

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

#include "mathlib.hpp"
#include "kernels/common/shared_structures.h"

bool Bounds3::Intersects(const Triangle &triangle) const
{
    float3 boxNormals[3] = {
        float3(1.0f, 0.0f, 0.0f),
        float3(0.0f, 1.0f, 0.0f),
        float3(0.0f, 0.0f, 1.0f)
    };
    
    // Test 1: Test the AABB against the minimal AABB around the triangle
    float triangleMin, triangleMax;
    for (int i = 0; i < 3; ++i)
    {
        triangle.Project(boxNormals[i], triangleMin, triangleMax);
        if (triangleMax < min[i] || triangleMin > max[i])
        {
            return false;
        }
    }
    
    // Test 2: Plane/AABB overlap test
    //float3 triangleNormal = Cross(triangle.p2 - triangle.p1, triangle.p3 - triangle.p1).normalize();
    //float triangleOffset = Dot(triangleNormal, triangle.p1);
    float boxMin, boxMax;
    /*
    Project(triangleNormal, boxMin, boxMax);
    if (boxMax < triangleOffset || boxMin > triangleOffset)
    {
    // return false;
    }
    */
    // Test 3: Edges test
    float3 triangleEdges[3] =
    {
        triangle.v1.position - triangle.v2.position,
        triangle.v2.position - triangle.v3.position,
        triangle.v3.position - triangle.v1.position
    };

    for (size_t i = 0; i < 3; ++i)
    {
        for (size_t j = 0; j < 3; ++j)
        {
            float3 axis = Cross(triangleEdges[i], boxNormals[j]);
            if (axis.Length() > 0.00001f)
            {
                Project(axis, boxMin, boxMax);
                triangle.Project(axis, triangleMin, triangleMax);
                if (boxMax < triangleMin || boxMin > triangleMax)
                {
                    return false;
                }
            }
        }
    }

    // No separating axis found
    return true;
}

void Bounds3::Project(float3 axis, float &mins, float &maxs) const
{
    mins = CL_FLT_MAX;
    maxs = -CL_FLT_MAX;

    float3 points[8] = {
        min,
        float3(min.x, min.y, max.z),
        float3(min.x, max.y, min.z),
        float3(min.x, max.y, max.z),
        float3(max.x, min.y, min.z),
        float3(max.x, min.y, max.z),
        float3(max.x, max.y, min.z),
        max
    };

    for (size_t i = 0; i < 8; ++i)
    {
        float val = Dot(points[i], axis);
        mins = std::min(mins, val);
        maxs = std::max(maxs, val);
    }
}
