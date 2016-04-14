#ifndef SCENE_HPP
#define SCENE_HPP

#include "vectors.hpp"
#include "aabb.hpp"
#include "triangle.hpp"
#include <CL/cl.hpp>
#include <algorithm>
#include <vector>

struct CellData
{
	cl_uint start_index;
	cl_uint	count;

};

class Scene
{
public:
    Scene(const char* filename, size_t cell_resolution);
    void LoadTriangles();
    void CreateGrid(size_t resolution, std::vector<cl_uint> &indices, std::vector<CellData> &cells);

    std::vector<Triangle> triangles;
    std::vector<cl_uint>  indices;
    std::vector<CellData> cells;

private:
    const char* filename_;

};

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

#endif // SCENE_HPP
