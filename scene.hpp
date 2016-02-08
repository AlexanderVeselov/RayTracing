#ifndef SCENE_HPP
#define SCENE_HPP

#include "vectors.hpp"
#include <CL/cl.h>

// Purpose: Base primitive in raytracer
class Sphere
{
public:
	Sphere(float3 Position, float Radius) :
		pos(Position),
		color(Position.normalize()),
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

	bool SphereIntersect(const Sphere &sphere);
	
private:
	float3 min, max;

};

#endif // SCENE_HPP
