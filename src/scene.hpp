#ifndef SCENE_HPP
#define SCENE_HPP

#include "vectors.hpp"
#include <CL/cl.hpp>
#include <algorithm>
#include <vector>

// Purpose: Base primitive in raytracer
class Sphere
{
public:
	Sphere(float3 Position, float Radius) :
		pos(Position),
		color(float3(Position.z, Position.y, Position.x).normalize()),
		r(Radius)
	{}

	// Getters...
	float3 GetPosition() const { return pos;	}
	float3 GetColor()    const { return color;	}
	float  GetRadius()   const { return r;		}

private:
	float3 pos;
	float3 color;
	float  r;
	// Padded to align on 4 floats due to technical reasons
	float  unused[3];

};

// Purpose: 
class Triangle
{
public:
    Triangle(float3 a, float3 b, float3 c, float3 na, float3 nb, float3 nc) :
      p1(a), p2(b), p3(c), n1(na), n2(nb), n3(nc)
    {
    }

    void Project(float3 axis, float &fMin, float &fMax) const
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

// Purpose: Axis-aligned Bounding box
class AABB
{
public:
	AABB(float3 PosMin, float3 PosMax) : 
		min(PosMin), max(PosMax)
	{}

	// Getters
	float3 GetMin() const { return min; }
	float3 GetMax() const { return max; }

	bool SphereIntersect(const Sphere &sphere) const;
    bool TriangleIntersect(const Triangle &triangle) const;
	
    void Project(float3 axis, float &fMin, float &fMax) const
    {
        fMin = CL_FLT_MAX;
        fMax = -CL_FLT_MAX;

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
            float val = dot(points[i], axis);
            fMin = std::min(fMin, val);
            fMax = std::max(fMax, val);
        }
    }

private:
	float3 min, max;

};

void LoadTriangles(const char* filename, std::vector<Triangle> &triangles);

#endif // SCENE_HPP
