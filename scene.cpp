#include "scene.hpp"
#include <cmath>

float sqr(float x)
{
	return x*x;
}

bool AABB::SphereIntersect(const Sphere &sphere)
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
