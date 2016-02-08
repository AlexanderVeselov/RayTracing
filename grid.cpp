#include "scene.hpp"
#include "grid.hpp"
#include <iostream>
#include <cmath>

// For sphere
void CreateGrid(const std::vector<Sphere> &objects, size_t resolution, std::vector<cl_uint> &indices, std::vector<CellData> &cells)
{
	typedef Sphere obj_t;
	typedef std::vector<obj_t>::const_iterator s_iter_t;

	std::cout << "Creating uniform grid..." << std::endl;
	
	// Use <algorithm>?
	float3 max_pos(0.0f), min_pos(0.0f);
	float max_r = 0.0f;
	for (s_iter_t it = objects.begin(); it != objects.end(); ++it)
	{
		max_pos.x = std::max(max_pos.x, it->GetPosition().x);
		max_pos.y = std::max(max_pos.y, it->GetPosition().y);
		max_pos.z = std::max(max_pos.z, it->GetPosition().z);
		
		min_pos.x = std::min(min_pos.x, it->GetPosition().x);
		min_pos.y = std::min(min_pos.y, it->GetPosition().y);
		min_pos.z = std::min(min_pos.z, it->GetPosition().z);

		max_r	  = std::max(max_r, it->GetRadius());
	}

	//AABB scene_box(min_pos - max_r, max_pos + max_r);
	AABB scene_box(float3(0.0f), float3(400));
	
	std::cout << "Create bounding box with\nmin = ("
		<< scene_box.GetMin().x << ", " << scene_box.GetMin().y << ", " << scene_box.GetMin().z << "), max = ("
		<< scene_box.GetMax().x << ", " << scene_box.GetMax().y << ", " << scene_box.GetMax().z << ")\n";
	std::cout << "Max radius: " << max_r << "\n";
	
	float InvResolution = 1.0f / resolution;
	float3 dv = (scene_box.GetMax() - scene_box.GetMin()) / resolution;
	
	size_t curr_index = 0;
	CellData current_cell = {0, 0};
	
	int iterations = resolution * resolution * resolution;
	int curr_iteration = 0;
	for (size_t z = 0; z < resolution; ++z)
	for (size_t y = 0; y < resolution; ++y)
	for (size_t x = 0; x < resolution; ++x)
	{
		AABB curr_box(scene_box.GetMin() + float3(x*dv.x, y*dv.y, z*dv.z), scene_box.GetMin() + float3((x+1)*dv.x, (y+1)*dv.y, (z+1)*dv.z));
		/*
		std::cout << "Bounding box with\nmin = ("
			<< curr_box.GetMin().x << ", " << curr_box.GetMin().y << ", " << curr_box.GetMin().z << "), max = ("
			<< curr_box.GetMax().x << ", " << curr_box.GetMax().y << ", " << curr_box.GetMax().z << ")\n";
		*/
		//std::cout << "Cell " << x + y * resolution + z * resolution * resolution << " (x: " << x << ", y: " << ", z: " << z << ")\n";
		//std::cout << "Start index: " << curr_index << "\n";

		size_t obj_index = 0, obj_count = 0;
		for (s_iter_t it = objects.begin(); it != objects.end(); ++it, ++obj_index)
		{
			if (curr_box.SphereIntersect(*it))
			{
				//std::cout << "Intersect with sphere # " << obj_index << " at (" << it->GetPosition().x << ", " << it->GetPosition().y << ", " << it->GetPosition().z << ")\n";
				indices.push_back(obj_index);
				++obj_count;
			}
		}
		current_cell.start_index += current_cell.count;
		current_cell.count = obj_count;
		//std::cout << "Cell start index: " << current_cell.start_index << ", object count: " << current_cell.count << "\n";
		cells.push_back(current_cell);
		if ((++curr_iteration) % 100 == 0)
		{
			std::cout << static_cast<float>(curr_iteration) / static_cast<float>(iterations) * 100.0f << "%" << std::endl;
		}
	}
	std::cout << "Grid created\n";
	
}
