#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include "vectors.hpp"
#include <CL/cl.h>

// Purpose: 
class Triangle
{
public:
    Triangle(float3 a, float3 b, float3 c, float3 na, float3 nb, float3 nc) :
      p1(a), p2(b), p3(c), n1(na), n2(nb), n3(nc)
    {
    }

    void project(float3 axis, float &fMin, float &fMax) const
    {
        fMin = CL_FLT_MAX;
        fMax = -CL_FLT_MAX;

        float3 points[3] = {
            p1, p2, p3
        };

        for (size_t i = 0; i < 3; ++i)
        {
            float val = dot(points[i], axis);
            fMin = std::min(fMin, val);
            fMax = std::max(fMax, val);
        }
    }

    float3 p1, p2, p3;
    float3 n1, n2, n3;

};

#endif // TRIANGLE_HPP
