/*****************************************************************************
 MIT License

 Copyright(c) 2021 Alexander Veselov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this softwareand associated documentation files(the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions :

 The above copyright noticeand this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *****************************************************************************/

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "scene.hpp"
#include "mathlib/mathlib.hpp"
#include "render.hpp"
#include "utils/cl_exception.hpp"
#include "io/image_loader.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include <cctype>
#include <GL/glew.h>

Scene::Scene(const char* filename, CLContext& cl_context, float scale, bool flip_yz)
    : cl_context_(cl_context)
{
    cl_int status;

    std::uint32_t dummy_value = 0;
    dummy_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(std::uint32_t), &dummy_value, &status);
    ThrowIfFailed(status, "Failed to create dummy buffer");

    Load(filename, scale, flip_yz);
}

std::vector<Triangle>& Scene::GetTriangles()
{
    return triangles_;
}

void Scene::Load(const char* filename, float scale, bool flip_yz)
{
    std::cout << "Loading object file " << filename << std::endl;

    std::string path_to_folder = std::string(filename, std::string(filename).rfind('/') + 1);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, path_to_folder.c_str());

    if (!success)
    {
        throw std::runtime_error("Failed to load the scene!");
    }

    materials_.resize(materials.size());

    const float kGamma = 2.2f;
    const std::uint32_t kInvalidTextureIndex = 0xFFFFFFFF;

    for (std::uint32_t material_idx = 0; material_idx < materials.size(); ++material_idx)
    {
        auto& out_material = materials_[material_idx];
        auto const& in_material = materials[material_idx];

        // Convert from sRGB to linear
        out_material.diffuse_albedo.x = pow(in_material.diffuse[0], kGamma);
        out_material.diffuse_albedo.y = pow(in_material.diffuse[1], kGamma);
        out_material.diffuse_albedo.z = pow(in_material.diffuse[2], kGamma);
        out_material.diffuse_albedo.padding = kInvalidTextureIndex;

        if (!in_material.diffuse_texname.empty())
        {
            out_material.diffuse_albedo.padding = LoadTexture((path_to_folder + in_material.diffuse_texname).c_str());
        }

        out_material.specular_albedo.x = pow(in_material.specular[0], kGamma);
        out_material.specular_albedo.y = pow(in_material.specular[1], kGamma);
        out_material.specular_albedo.z = pow(in_material.specular[2], kGamma);
        out_material.specular_albedo.padding = kInvalidTextureIndex;

        if (!in_material.specular_texname.empty())
        {
            out_material.specular_albedo.padding = LoadTexture((path_to_folder + in_material.specular_texname).c_str());
        }

        out_material.emission.x = in_material.emission[0];
        out_material.emission.y = in_material.emission[1];
        out_material.emission.z = in_material.emission[2];
        out_material.emission.padding = kInvalidTextureIndex;

        if (!in_material.emissive_texname.empty())
        {
            out_material.emission.padding = LoadTexture((path_to_folder + in_material.emissive_texname).c_str());
        }

        out_material.roughness = in_material.roughness;
        out_material.roughness_idx = kInvalidTextureIndex;

        if (!in_material.roughness_texname.empty())
        {
            out_material.roughness_idx = LoadTexture((path_to_folder + in_material.roughness_texname).c_str());
        }

        out_material.metalness = in_material.metallic;

        out_material.ior = in_material.ior;
    }

    auto flip_vector = [](float3& vec, bool do_flip)
    {
        if (do_flip)
        {
            std::swap(vec.y, vec.z);
        }
    };

    for (auto const& shape : shapes)
    {
        auto const& indices = shape.mesh.indices;
        // The mesh is triangular
        assert(indices.size() % 3 == 0);

        for (std::uint32_t face = 0; face < indices.size() / 3; ++face)
        {
            auto pos_idx_1 = indices[face * 3 + 0].vertex_index;
            auto pos_idx_2 = indices[face * 3 + 1].vertex_index;
            auto pos_idx_3 = indices[face * 3 + 2].vertex_index;

            auto normal_idx_1 = indices[face * 3 + 0].normal_index;
            auto normal_idx_2 = indices[face * 3 + 1].normal_index;
            auto normal_idx_3 = indices[face * 3 + 2].normal_index;

            auto texcoord_idx_1 = indices[face * 3 + 0].texcoord_index;
            auto texcoord_idx_2 = indices[face * 3 + 1].texcoord_index;
            auto texcoord_idx_3 = indices[face * 3 + 2].texcoord_index;

            Vertex v1;
            v1.position.x = attrib.vertices[pos_idx_1 * 3 + 0] * scale;
            v1.position.y = attrib.vertices[pos_idx_1 * 3 + 1] * scale;
            v1.position.z = attrib.vertices[pos_idx_1 * 3 + 2] * scale;

            v1.normal.x = attrib.normals[normal_idx_1 * 3 + 0];
            v1.normal.y = attrib.normals[normal_idx_1 * 3 + 1];
            v1.normal.z = attrib.normals[normal_idx_1 * 3 + 2];

            v1.texcoord.x = texcoord_idx_1 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_1 * 2 + 0];
            v1.texcoord.y = texcoord_idx_1 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_1 * 2 + 1];

            Vertex v2;
            v2.position.x = attrib.vertices[pos_idx_2 * 3 + 0] * scale;
            v2.position.y = attrib.vertices[pos_idx_2 * 3 + 1] * scale;
            v2.position.z = attrib.vertices[pos_idx_2 * 3 + 2] * scale;

            v2.normal.x = attrib.normals[normal_idx_2 * 3 + 0];
            v2.normal.y = attrib.normals[normal_idx_2 * 3 + 1];
            v2.normal.z = attrib.normals[normal_idx_2 * 3 + 2];

            v2.texcoord.x = texcoord_idx_2 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_2 * 2 + 0];
            v2.texcoord.y = texcoord_idx_2 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_2 * 2 + 1];

            Vertex v3;
            v3.position.x = attrib.vertices[pos_idx_3 * 3 + 0] * scale;
            v3.position.y = attrib.vertices[pos_idx_3 * 3 + 1] * scale;
            v3.position.z = attrib.vertices[pos_idx_3 * 3 + 2] * scale;

            v3.normal.x = attrib.normals[normal_idx_3 * 3 + 0];
            v3.normal.y = attrib.normals[normal_idx_3 * 3 + 1];
            v3.normal.z = attrib.normals[normal_idx_3 * 3 + 2];

            v3.texcoord.x = texcoord_idx_3 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_3 * 2 + 0];
            v3.texcoord.y = texcoord_idx_3 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_3 * 2 + 1];

            flip_vector(v1.position, flip_yz);
            flip_vector(v1.normal, flip_yz);
            flip_vector(v2.position, flip_yz);
            flip_vector(v2.normal, flip_yz);
            flip_vector(v3.position, flip_yz);
            flip_vector(v3.normal, flip_yz);

            if (flip_yz)
            {
                std::swap(v1, v2);
            }

            if (shape.mesh.material_ids[face] >= 0 && shape.mesh.material_ids[face] < materials_.size())
            {
                triangles_.emplace_back(v1, v2, v3, shape.mesh.material_ids[face]);
            }
            else
            {
                // Use the default material
                triangles_.emplace_back(v1, v2, v3, 0);
            }
        }
    }

    std::cout << "Load successful (" << triangles_.size() << " triangles)" << std::endl;

}

std::size_t Scene::LoadTexture(char const* filename)
{
    // Try to lookup the cache
    auto it = loaded_textures_.find(filename);
    if (it != loaded_textures_.cend())
    {
        return it->second;
    }

    // Load the texture
    char const* file_extension = strrchr(filename, '.');
    if (file_extension == nullptr)
    {
        throw std::runtime_error("Invalid texture extension");
    }

    bool success = false;
    Image image;
    if (strcmp(file_extension, ".hdr") == 0)
    {
        assert(!"Not implemented yet!");
        success = LoadHDR(filename, image);
    }
    else if (strcmp(file_extension, ".jpg") == 0 || strcmp(file_extension, ".tga") == 0 || strcmp(file_extension, ".png") == 0)
    {
        success = LoadSTB(filename, image);
    }

    if (!success)
    {
        throw std::runtime_error((std::string("Failed to load file ") + filename).c_str());
    }

    Texture texture;
    texture.width = image.width;
    texture.height = image.height;
    texture.data_start = (std::uint32_t)texture_data_.size();

    std::size_t texture_idx = textures_.size();
    textures_.push_back(std::move(texture));

    texture_data_.insert(texture_data_.end(), image.data.begin(), image.data.end());

    // Cache the texture
    loaded_textures_.emplace(filename, texture_idx);
    return texture_idx;
}

void Scene::CollectEmissiveTriangles()
{
    for (auto triangle_idx = 0; triangle_idx < triangles_.size(); ++triangle_idx)
    {
        auto const& triangle = triangles_[triangle_idx];
        auto const& emission = materials_[triangle.mtlIndex].emission;

        if (emission.x + emission.y + emission.z > 0.0f)
        {
            // The triangle is emissive
            emissive_indices_.push_back(triangle_idx);
        }
    }

    scene_info_.emissive_count = (std::uint32_t)emissive_indices_.size();
}

void Scene::AddPointLight(float3 origin, float3 radiance)
{
    Light light = { origin, radiance, LIGHT_TYPE_POINT };
    lights_.push_back(std::move(light));
}

void Scene::AddDirectionalLight(float3 direction, float3 radiance)
{
    Light light = { direction.Normalize(), radiance, LIGHT_TYPE_DIRECTIONAL };
    lights_.emplace_back(std::move(light));
}

void Scene::Finalize()
{
    CollectEmissiveTriangles();

    //scene_info_.environment_map_index = LoadTexture("textures/studio_small_03_4k.hdr");
    scene_info_.analytic_light_count = (std::uint32_t)lights_.size();

    cl_int status;

    assert(!triangles_.empty());
    triangle_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        triangles_.size() * sizeof(Triangle), triangles_.data(), &status);
    ThrowIfFailed(status, "Failed to create scene buffer");

    assert(!materials_.empty());
    material_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        materials_.size() * sizeof(Material), materials_.data(), &status);
    ThrowIfFailed(status, "Failed to create material buffer");

    if (!emissive_indices_.empty())
    {
        emissive_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            emissive_indices_.size() * sizeof(std::uint32_t), emissive_indices_.data(), &status);
        ThrowIfFailed(status, "Failed to create emissive buffer");
    }

    if (!lights_.empty())
    {
        analytic_light_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            lights_.size() * sizeof(Light), lights_.data(), &status);
        ThrowIfFailed(status, "Failed to create analytic light buffer");
    }

    if (!textures_.empty())
    {
        texture_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            textures_.size() * sizeof(Texture), textures_.data(), &status);
        ThrowIfFailed(status, "Failed to create texture buffer");
    }

    if (!texture_data_.empty())
    {
        texture_data_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            texture_data_.size() * sizeof(std::uint32_t), texture_data_.data(), &status);
        ThrowIfFailed(status, "Failed to create texture data buffer");
    }

    ///@TODO: remove from here
    // Texture Buffers
    cl::ImageFormat image_format;
    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_FLOAT;

    Image image;
    LoadHDR("assets/ibl/Topanga_Forest_B_3k.hdr", image);
    env_texture_ = cl::Image2D(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        image_format, image.width, image.height, 0, image.data.data(), &status);
    ThrowIfFailed(status, "Failed to create environment image");
}
