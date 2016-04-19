#include "scene.hpp"
#include "math_functions.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

Scene::Scene(const char* filename, size_t cell_resolution) : filename_(filename), cell_resolution_(cell_resolution)
{
    LoadTriangles();
	CreateGrid(cell_resolution_, indices, cells);
}

void Scene::CreateGrid(size_t resolution, std::vector<cl_uint> &indices, std::vector<CellData> &cells)
{
    std::vector<cl_uint> lower_indices;
    std::vector<CellData> lower_cells;

    if (resolution > 2)
    {
        CreateGrid(resolution / 2, lower_indices, lower_cells);
    }
    
	typedef std::vector<Triangle>::const_iterator iter_t;

	std::cout << "Creating uniform grid with resolution " << resolution << "..." << std::endl;
	
	Aabb scene_box(float3(0.0f), float3(64.0f));
	
	float InvResolution = 1.0f / static_cast<float>(resolution);
	float3 dv = (scene_box.getMax() - scene_box.getMin()) / static_cast<float>(resolution);
	
	size_t curr_index = 0;
	CellData current_cell = {0, 0};
	
	int iterations = resolution * resolution * resolution;
	int curr_iteration = 0;
	for (size_t z = 0; z < resolution; ++z)
	for (size_t y = 0; y < resolution; ++y)
	for (size_t x = 0; x < resolution; ++x)
	{
		Aabb curr_box(scene_box.getMin() + float3(x*dv.x, y*dv.y, z*dv.z), scene_box.getMin() + float3((x+1)*dv.x, (y+1)*dv.y, (z+1)*dv.z));

		size_t obj_index = 0, obj_count = 0;
        if (resolution == 2)
        {
		    for (iter_t it = triangles.begin(); it != triangles.end(); ++it, ++obj_index)
		    {
                if (curr_box.triangleIntersect(*it))
			    {
				    indices.push_back(obj_index);
				    ++obj_count;
			    }
		    }
        }
        else
        {
            CellData lower_cell = lower_cells[x / 2 + (y / 2) * (resolution / 2) + (z / 2) * (resolution / 2) * (resolution / 2)];
            for (size_t i = lower_cell.start_index; i < lower_cell.start_index + lower_cell.count; ++i, ++obj_index)
            {
                if (curr_box.triangleIntersect(triangles[lower_indices[i]]))
                {
                    indices.push_back(lower_indices[i]);
                    ++obj_count;
                }
            }
        }
		current_cell.start_index += current_cell.count;
		current_cell.count = obj_count;

		cells.push_back(current_cell);

        
		if ((++curr_iteration) % 100000 == 0)
		{
			std::cout << static_cast<float>(curr_iteration) / static_cast<float>(iterations) * 100.0f << "%" << std::endl;
		}
        
	}
	std::cout << "Grid created" << std::endl;
	
}

void Scene::LoadTriangles()
{
    std::ifstream input_file(filename_);

    std::cout << "Loading object file " << filename_ << std::endl;
	
    if (!input_file)
	{
		std::cerr << "Cannot load object file " << filename_ << std::endl;
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
