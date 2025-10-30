/*****************************************************************************
 MIT License

 Copyright(c) 2023 Alexander Veselov

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

#include <GL/glew.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "scene.hpp"
#include "mathlib/mathlib.hpp"
#include "render.hpp"
#include "utils/cl_exception.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include <cctype>
#include <filesystem>

#undef max

Scene::Scene(const char* filename, float scale, bool flip_yz)
{
    Load(filename, scale, flip_yz);
}

namespace
{
unsigned int PackAlbedo(float r, float g, float b, std::uint32_t texture_index)
{
    assert(texture_index < 256);
    r = clamp(r, 0.0f, 1.0f);
    g = clamp(g, 0.0f, 1.0f);
    b = clamp(b, 0.0f, 1.0f);
    return ((unsigned int)(r * 255.0f)) | ((unsigned int)(g * 255.0f) << 8)
         | ((unsigned int)(b * 255.0f) << 16) | (texture_index << 24);
}

unsigned int PackRGBE(float r, float g, float b)
{
    // Make sure the values are not negative
    r = std::max(r, 0.0f);
    g = std::max(g, 0.0f);
    b = std::max(b, 0.0f);

    float v = r;
    if (g > v) v = g;
    if (b > v) v = b;

    if (v < 1e-32f)
    {
        return 0;
    }
    else
    {
        int e;
        v = frexp(v, &e) * 256.0f / v;
        return ((unsigned int)(r * v)) | ((unsigned int)(g * v) << 8)
            | ((unsigned int)(b * v) << 16) | ((e + 128) << 24);
    }
}

float3 UnpackRGBE(unsigned int rgbe)
{
    float f;
    int r = (rgbe >> 0) & 0xFF;
    int g = (rgbe >> 8) & 0xFF;
    int b = (rgbe >> 16) & 0xFF;
    int exp = rgbe >> 24;

    if (exp)
    {   /*nonzero pixel*/
        f = ldexp(1.0f, exp - (int)(128 + 8));
        return float3((float)r, (float)g, (float)b) * f;
    }
    else
    {
        return 0.0;
    }
}

unsigned int PackRoughnessMetalness(float roughness, std::uint32_t roughness_idx,
    float metalness, std::uint32_t metalness_idx)
{
    assert(roughness_idx < 256 && metalness_idx < 256);
    roughness = clamp(roughness, 0.0f, 1.0f);
    metalness = clamp(metalness, 0.0f, 1.0f);
    return ((unsigned int)(roughness * 255.0f)) | (roughness_idx << 8)
        | ((unsigned int)(metalness * 255.0f) << 16) | (metalness_idx << 24);
}

unsigned int PackIorEmissionIdxTransparency(float ior, std::uint32_t emission_idx,
    float transparency, std::uint32_t transparency_idx)
{
    assert(emission_idx < 256 && transparency_idx < 256);
    ior = clamp(ior, 0.0f, 10.0f);
    transparency = clamp(transparency, 0.0f, 1.0f);
    return ((unsigned int)(ior * 25.5f)) | (emission_idx << 8)
        | ((unsigned int)(transparency * 255.0f) << 16) | (transparency_idx << 24);
}
}

void Scene::Load(const char* filename, float scale, bool flip_yz)
{
    std::cout << "Loading object file " << filename << std::endl;

    std::string path_to_folder = std::filesystem::path(filename).parent_path().string();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, path_to_folder.c_str());
    if (!success) {
        throw std::runtime_error("Failed to load the scene!");
    }

    // Collect materials
    materials_.resize(materials.size());
    const float kGamma = 2.2f;
    const std::uint32_t kInvalidTextureIndex = 0xFF;

    for (std::uint32_t material_idx = 0; material_idx < materials.size(); ++material_idx) {
        auto& out_material = materials_[material_idx];
        auto const& in_material = materials[material_idx];

        out_material.diffuse_albedo = PackAlbedo(
            pow(in_material.diffuse[0], kGamma),
            pow(in_material.diffuse[1], kGamma),
            pow(in_material.diffuse[2], kGamma),
            in_material.diffuse_texname.empty() ? kInvalidTextureIndex :
            LoadTexture((path_to_folder + "/" + in_material.diffuse_texname).c_str()));

        out_material.specular_albedo = PackAlbedo(
            pow(in_material.specular[0], kGamma),
            pow(in_material.specular[1], kGamma),
            pow(in_material.specular[2], kGamma),
            in_material.specular_texname.empty() ? kInvalidTextureIndex :
            LoadTexture((path_to_folder + "/" + in_material.specular_texname).c_str()));

        out_material.emission = PackRGBE(in_material.emission[0], in_material.emission[1], in_material.emission[2]);

        out_material.roughness_metalness = PackRoughnessMetalness(
            in_material.roughness,
            in_material.roughness_texname.empty() ? kInvalidTextureIndex :
            LoadTexture((path_to_folder + "/" + in_material.roughness_texname).c_str()),
            in_material.metallic,
            in_material.metallic_texname.empty() ? kInvalidTextureIndex :
            LoadTexture((path_to_folder + "/" + in_material.metallic_texname).c_str()));

        out_material.ior_emission_idx_transparency = PackIorEmissionIdxTransparency(
            in_material.ior,
            in_material.emissive_texname.empty() ? kInvalidTextureIndex :
            LoadTexture((path_to_folder + "/" + in_material.emissive_texname).c_str()),
            in_material.transmittance[0],
            in_material.alpha_texname.empty() ? kInvalidTextureIndex :
            LoadTexture((path_to_folder + "/" + in_material.alpha_texname).c_str()));
    }

    auto flip_vector = [](float3& v, bool do_flip) {
        if (do_flip) {
            std::swap(v.y, v.z);
            v.y = -v.y;
        }
        };

    // Reserve memory (approx count) to reduce re-allocations
    size_t approx_triangles = 0;
    for (auto const& s : shapes) approx_triangles += s.mesh.indices.size() / 3;
    vertices_.reserve(vertices_.size() + approx_triangles * 3);
    indices_.reserve(indices_.size() + approx_triangles * 3);
    material_ids_.reserve(material_ids_.size() + approx_triangles);

    // Build VB/IB
    for (auto const& shape : shapes)
    {
        const auto& indices = shape.mesh.indices;
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

            Vertex v1{}, v2{}, v3{};

            // positions
            v1.position = float3(
                attrib.vertices[pos_idx_1 * 3 + 0] * scale,
                attrib.vertices[pos_idx_1 * 3 + 1] * scale,
                attrib.vertices[pos_idx_1 * 3 + 2] * scale);
            v2.position = float3(
                attrib.vertices[pos_idx_2 * 3 + 0] * scale,
                attrib.vertices[pos_idx_2 * 3 + 1] * scale,
                attrib.vertices[pos_idx_2 * 3 + 2] * scale);
            v3.position = float3(
                attrib.vertices[pos_idx_3 * 3 + 0] * scale,
                attrib.vertices[pos_idx_3 * 3 + 1] * scale,
                attrib.vertices[pos_idx_3 * 3 + 2] * scale);

            // normals (fallback to face normal if there are no normals in OBJ)
            auto compute_face_normal = [](const float3& a, const float3& b, const float3& c) {
                return (Cross(b - a, c - a)).Normalize();
                };
            bool has_n = (normal_idx_1 >= 0 && normal_idx_2 >= 0 && normal_idx_3 >= 0);

            if (has_n) {
                v1.normal = float3(
                    attrib.normals[normal_idx_1 * 3 + 0],
                    attrib.normals[normal_idx_1 * 3 + 1],
                    attrib.normals[normal_idx_1 * 3 + 2]);
                v2.normal = float3(
                    attrib.normals[normal_idx_2 * 3 + 0],
                    attrib.normals[normal_idx_2 * 3 + 1],
                    attrib.normals[normal_idx_2 * 3 + 2]);
                v3.normal = float3(
                    attrib.normals[normal_idx_3 * 3 + 0],
                    attrib.normals[normal_idx_3 * 3 + 1],
                    attrib.normals[normal_idx_3 * 3 + 2]);
            }
            else {
                float3 n = compute_face_normal(v1.position, v2.position, v3.position);
                v1.normal = v2.normal = v3.normal = n;
            }

            // texcoords
            v1.texcoord = (texcoord_idx_1 < 0) ? float2(0.0f) :
                float2(attrib.texcoords[texcoord_idx_1 * 2 + 0], attrib.texcoords[texcoord_idx_1 * 2 + 1]);
            v2.texcoord = (texcoord_idx_2 < 0) ? float2(0.0f) :
                float2(attrib.texcoords[texcoord_idx_2 * 2 + 0], attrib.texcoords[texcoord_idx_2 * 2 + 1]);
            v3.texcoord = (texcoord_idx_3 < 0) ? float2(0.0f) :
                float2(attrib.texcoords[texcoord_idx_3 * 2 + 0], attrib.texcoords[texcoord_idx_3 * 2 + 1]);

            // YZ flip (позиции и нормали)
            flip_vector(v1.position, flip_yz);
            flip_vector(v2.position, flip_yz);
            flip_vector(v3.position, flip_yz);
            flip_vector(v1.normal, flip_yz);
            flip_vector(v2.normal, flip_yz);
            flip_vector(v3.normal, flip_yz);

            // пишем 3 вершины подряд
            const std::uint32_t base = static_cast<std::uint32_t>(vertices_.size());
            vertices_.push_back(v1);
            vertices_.push_back(v2);
            vertices_.push_back(v3);

            // и 3 индекса на них
            indices_.push_back(base + 0);
            indices_.push_back(base + 1);
            indices_.push_back(base + 2);

            // материал для этого треугольника
            std::uint32_t mtl = 0;
            if (face < shape.mesh.material_ids.size()) {
                int mid = shape.mesh.material_ids[face];
                if (mid >= 0 && static_cast<size_t>(mid) < materials_.size()) mtl = static_cast<std::uint32_t>(mid);
            }
            material_ids_.push_back(mtl);
        }
    }

    const auto tri_count = static_cast<std::uint32_t>(indices_.size() / 3);
    assert(material_ids_.size() == (std::size_t)tri_count);

    for (std::uint32_t material_id : material_ids_)
    {
        assert(material_id < materials_.size());
    }

    std::cout << "Load successful (" << tri_count << " triangles, "
        << vertices_.size() << " vertices, "
        << indices_.size() << " indices)" << std::endl;
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
    emissive_indices_.clear();
    emissive_indices_.reserve(material_ids_.size());

    for (std::uint32_t t = 0; t < material_ids_.size(); ++t)
    {
        std::uint32_t mid = material_ids_[t];
        if (mid < materials_.size())
        {
            float3 e = UnpackRGBE(materials_[mid].emission);
            if ((e.x + e.y + e.z) > 0.0f) emissive_indices_.push_back(t);
        }
    }
    scene_info_.emissive_count = static_cast<std::uint32_t>(emissive_indices_.size());
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

    LoadHDR("assets/ibl/CGSkies_0036_free.hdr", env_image_);
}
