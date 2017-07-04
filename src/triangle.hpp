#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include "vectors.hpp"
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

class Triangle
{
public:
    Triangle(float3 a, float3 b, float3 c, float3 na, float3 nb, float3 nc, Material mtl) :
      p1(a), p2(b), p3(c), n1(na), n2(nb), n3(nc), material(mtl)
    {
    }

    void Project(float3 axis, float &min, float &max) const
    {
		min = CL_FLT_MAX;
		max = -CL_FLT_MAX;

        float3 points[3] =
		{
            p1, p2, p3
        };

        for (size_t i = 0; i < 3; ++i)
        {
            float val = dot(points[i], axis);
			min = std::min(min, val);
			max = std::max(max, val);
        }
    }

    float3 p1, p2, p3;
    float3 n1, n2, n3;
    Material material;

};

#endif // TRIANGLE_HPP
