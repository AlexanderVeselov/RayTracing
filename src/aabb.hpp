#ifndef AABB_HPP
#define AABB_HPP

#include "vectors.hpp"
#include "triangle.hpp"

// Purpose: Axis-aligned Bounding box
class Aabb
{
public:
	Aabb(float3 min, float3 max) : 
		min_(min), max_(max)
	{}

	// Getters
	float3 getMin() const { return min_; }
	float3 getMax() const { return max_; }

	//bool SphereIntersect(const Sphere &sphere) const;
    bool triangleIntersect(const Triangle &triangle) const;
	
    void project(float3 axis, float &fMin, float &fMax) const;

private:
	float3 min_, max_;

};

#endif // AABB_HPP
