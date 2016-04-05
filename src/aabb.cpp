#include "aabb.hpp"
#include <CL/cl.h>

bool Aabb::triangleIntersect(const Triangle &triangle) const
{
    float3 boxNormals[3] = {
        float3(1.0f, 0.0f, 0.0f),
        float3(0.0f, 1.0f, 0.0f),
        float3(0.0f, 0.0f, 1.0f)
    };
    
    //logout << "Test triangle with p1" << triangle.p1 << " p2" << triangle.p2 << " p3" << triangle.p3 << "\n";
    // Test 1: Test the AABB against the minimal AABB around the triangle
    float triangleMin, triangleMax;
    for (int i = 0; i < 3; ++i)
    {
        triangle.project(boxNormals[i], triangleMin, triangleMax);
        if (triangleMax < min_[i] || triangleMin > max_[i])
        {
            return false;
        }
    }
    
    
    // Test 2: Plane/AABB overlap test
    //float3 triangleNormal = cross(triangle.p2 - triangle.p1, triangle.p3 - triangle.p1).normalize();
    //float triangleOffset = dot(triangleNormal, triangle.p1);
    float boxMin, boxMax;
    /*
    Project(triangleNormal, boxMin, boxMax);
    if (boxMax < triangleOffset || boxMin > triangleOffset)
    {
       // return false;
    }
    */
    // Test 3: Edges test
    float3 triangleEdges[3] = {
        triangle.p1 - triangle.p2,
        triangle.p2 - triangle.p3,
        triangle.p3 - triangle.p1
    };

    for (size_t i = 0; i < 3; ++i)
    {
        for (size_t j = 0; j < 3; ++j)
        {
            float3 axis = cross(triangleEdges[i], boxNormals[j]);
            if (axis.length() > 0.00001f)
            {
                project(axis, boxMin, boxMax);
                triangle.project(axis, triangleMin, triangleMax);
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

void Aabb::project(float3 axis, float &fMin, float &fMax) const
{
    fMin = CL_FLT_MAX;
    fMax = -CL_FLT_MAX;

    float3 points[8] = {
        min_,
        float3(min_.x, min_.y, max_.z),
        float3(min_.x, max_.y, min_.z),
        float3(min_.x, max_.y, max_.z),
        float3(max_.x, min_.y, min_.z),
        float3(max_.x, min_.y, max_.z),
        float3(max_.x, max_.y, min_.z),
        max_
    };

    for (size_t i = 0; i < 8; ++i)
    {
        float val = dot(points[i], axis);
        fMin = std::min(fMin, val);
        fMax = std::max(fMax, val);
    }
}

/*
bool AABB::SphereIntersect(const Sphere &sphere) const
{
	float3 s_pos = sphere.GetPosition();
	float dist_sqr = 0.0f;

	if (s_pos.x < min.x)
		dist_sqr += sqr(s_pos.x - min.x);
	else if (s_pos.x > max.x)
		dist_sqr += sqr(s_pos.x - max.x);

	if (s_pos.y < min.y)
		dist_sqr += sqr(s_pos.y - min.y);
	else if (s_pos.y > max.y)
		dist_sqr += sqr(s_pos.y - max.y);

	if (s_pos.z < min.z)
		dist_sqr += sqr(s_pos.z - min.z);
	else if (s_pos.z > max.z)
		dist_sqr += sqr(s_pos.z - max.z);

	return dist_sqr < sqr(sphere.GetRadius());
}
*/
