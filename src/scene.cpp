#include "scene.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

float sqr(float x)
{
	return x*x;
}

void LoadTriangles(const char* filename, std::vector<Triangle> &triangles)
{
    std::ifstream input_file(filename);

    std::cout << "Loading object file " << filename << std::endl;
	
    if (!input_file)
	{
		std::cerr << "Cannot load object file " << filename << std::endl;
		return;
	}
    
    std::vector<float3> vertices;
    std::vector<float3> normals;

    std::string curr_line;
    while (std::getline(input_file, curr_line))
	{
        if (curr_line.size() == 0)
        {
            continue;
        }

        if (curr_line[0] == 'v' && curr_line[1] == ' ')
        {
            curr_line.erase(0, 3);
            float x = std::stof(curr_line.substr(0, curr_line.find(' ')));
            curr_line.erase(0, curr_line.find(' ') + 1);
            float y = std::stof(curr_line.substr(0, curr_line.find(' ')));
            curr_line.erase(0, curr_line.find(' ') + 1);
            float z = std::stof(curr_line.substr(0, curr_line.find(' ')));
            vertices.push_back(float3(x, y, z));
        }

        if (curr_line[0] == 'v' && curr_line[1] == 'n')
        {
            curr_line.erase(0, 3);
            float x = std::stof(curr_line.substr(0, curr_line.find(' ')));
            curr_line.erase(0, curr_line.find(' ') + 1);
            float y = std::stof(curr_line.substr(0, curr_line.find(' ')));
            curr_line.erase(0, curr_line.find(' ') + 1);
            float z = std::stof(curr_line.substr(0, curr_line.find(' ')));
            normals.push_back(float3(x, y, z));
        }

        if (curr_line[0] == 'f')
        {
            curr_line.erase(0, 2);
            int iv[3], it[3], in[3];
            for (size_t i = 0; i < 3; ++i)
            {
                iv[i] = std::stoi(curr_line.substr(0, curr_line.find('/'))) - 1;
                curr_line.erase(0, curr_line.find('/') + 1);
                it[i] = std::stoi(curr_line.substr(0, curr_line.find('/'))) - 1;
                curr_line.erase(0, curr_line.find('/') + 1);
                in[i] = std::stoi(curr_line.substr(0, curr_line.find('/'))) - 1;
                curr_line.erase(0, curr_line.find(' ') + 1);

            }

            triangles.push_back(Triangle(vertices[iv[0]], vertices[iv[1]], vertices[iv[2]], normals[in[0]], normals[in[1]], normals[in[2]]));

        }
	}

    std::cout << "Load succesful (" << triangles.size() << " triangles)" << std::endl;

}

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

bool AABB::TriangleIntersect(const Triangle &triangle) const
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
        triangle.Project(boxNormals[i], triangleMin, triangleMax);
        if (triangleMax < min[i] || triangleMin > max[i])
        {
            return false;
        }
    }
    
    /*
    // Test 2: Plane/AABB overlap test
    float3 triangleNormal = cross(triangle.p2 - triangle.p1, triangle.p3 - triangle.p1).normalize();
    float triangleOffset = dot(triangleNormal, triangle.p1);
    float boxMin, boxMax;
    Project(triangleNormal, boxMin, boxMax);
    if (boxMax < triangleOffset || boxMin > triangleOffset)
    {
        return false;
    }
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
                Project(axis, boxMin, boxMax);
                triangle.Project(axis, triangleMin, triangleMax);
                if (boxMax < triangleMin || boxMin > triangleMax)
                {
                    return false;
                }
            }
        }
    }
    */
    // No separating axis found
    return true;
}
