#ifndef AABB_HPP
#define AABB_HPP

#include "mathlib.hpp"
#include "triangle.hpp"

// Purpose: Axis-aligned Bounding box
struct Aabb
{
public:
    Aabb(float3 mins, float3 maxs) : 
        min(mins), max(maxs)
    {}


    //bool SphereIntersect(const Sphere &sphere) const;
    bool Intersects(const Triangle &triangle) const;
    
    void Project(float3 axis, float &mins, float &maxs) const;

    float3 min;
    float3 max;

};

#endif // AABB_HPP
