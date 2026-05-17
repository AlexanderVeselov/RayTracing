/*****************************************************************************
 MIT License

 Copyright(c) 2026 Alexander Veselov

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

#include "render.hpp"
#include "scene.hpp"
#include "utils/cl_exception.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#undef max

Scene::Scene(const char* filename, float scale, bool flip_yz)
{
    Load(filename, scale, flip_yz);
}

namespace
{
// Simple FNV-1a hash combiner for 32-bit values
inline size_t hash_combine_u32(size_t h, uint32_t v)
{
    h ^= v;
    h *= 16777619u;  // FNV prime
    return h;
}

struct VertexHasher
{
    size_t operator()(Vertex const& v) const noexcept
    {
        size_t h = 2166136261u;  // FNV offset basis
        auto hash_float = [&](float f)
        {
            uint32_t bits;
            static_assert(sizeof(bits) == sizeof(f), "Unexpected float size");
            std::memcpy(&bits, &f, sizeof(f));
            h = hash_combine_u32(h, bits);
        };
        hash_float(v.position.x);
        hash_float(v.position.y);
        hash_float(v.position.z);
        hash_float(v.normal.x);
        hash_float(v.normal.y);
        hash_float(v.normal.z);
        hash_float(v.texcoord.x);
        hash_float(v.texcoord.y);
        return h;
    }
};

struct VertexEqual
{
    bool operator()(Vertex const& a, Vertex const& b) const noexcept
    {
        return a.position.x == b.position.x && a.position.y == b.position.y && a.position.z == b.position.z
            && a.normal.x == b.normal.x && a.normal.y == b.normal.y && a.normal.z == b.normal.z
            && a.texcoord.x == b.texcoord.x && a.texcoord.y == b.texcoord.y;
    }
};

unsigned int PackAlbedo(float r, float g, float b, uint32_t texture_index)
{
    assert(texture_index < 256);
    r = std::clamp(r, 0.0f, 1.0f);
    g = std::clamp(g, 0.0f, 1.0f);
    b = std::clamp(b, 0.0f, 1.0f);
    return ((unsigned int)(r * 255.0f)) | ((unsigned int)(g * 255.0f) << 8) | ((unsigned int)(b * 255.0f) << 16)
        | (texture_index << 24);
}

unsigned int PackRGBE(float r, float g, float b)
{
    // Make sure the values are not negative
    r = std::max(r, 0.0f);
    g = std::max(g, 0.0f);
    b = std::max(b, 0.0f);

    float v = r;
    if (g > v)
        v = g;
    if (b > v)
        v = b;

    if (v < 1e-32f)
    {
        return 0;
    }
    else
    {
        int e;
        v = frexp(v, &e) * 256.0f / v;
        return ((unsigned int)(r * v)) | ((unsigned int)(g * v) << 8) | ((unsigned int)(b * v) << 16)
            | ((e + 128) << 24);
    }
}

glm::vec3 UnpackRGBE(unsigned int rgbe)
{
    float f;
    int r = (rgbe >> 0) & 0xFF;
    int g = (rgbe >> 8) & 0xFF;
    int b = (rgbe >> 16) & 0xFF;
    int exp = rgbe >> 24;

    if (exp)
    { /*nonzero pixel*/
        f = ldexp(1.0f, exp - (int)(128 + 8));
        return glm::vec3((float)r, (float)g, (float)b) * f;
    }
    else
    {
        return glm::vec3(0.0f);
    }
}

unsigned int PackRoughnessMetalness(float roughness, uint32_t roughness_idx, float metalness,
    uint32_t metalness_idx)
{
    assert(roughness_idx < 256 && metalness_idx < 256);
    roughness = std::clamp(roughness, 0.0f, 1.0f);
    metalness = std::clamp(metalness, 0.0f, 1.0f);
    return ((unsigned int)(roughness * 255.0f)) | (roughness_idx << 8) | ((unsigned int)(metalness * 255.0f) << 16)
        | (metalness_idx << 24);
}

unsigned int PackIorEmissionIdxTransparency(float ior, uint32_t emission_idx, float transparency,
    uint32_t transparency_idx)
{
    assert(emission_idx < 256 && transparency_idx < 256);
    ior = std::clamp(ior, 0.0f, 10.0f);
    transparency = std::clamp(transparency, 0.0f, 1.0f);
    return ((unsigned int)(ior * 25.5f)) | (emission_idx << 8) | ((unsigned int)(transparency * 255.0f) << 16)
        | (transparency_idx << 24);
}
}  // namespace

void Scene::Load(const char* filename, float scale, bool flip_yz)
{
    std::cout << "Loading object file " << filename << std::endl;

    std::string path_to_folder = std::filesystem::path(filename).parent_path().string();

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
    const uint32_t kInvalidTextureIndex = 0xFF;

    for (uint32_t material_idx = 0; material_idx < materials.size(); ++material_idx)
    {
        auto& out_material = materials_[material_idx];
        auto const& in_material = materials[material_idx];

        // Convert from sRGB to linear
        out_material.diffuse_albedo = PackAlbedo(pow(in_material.diffuse[0], kGamma),  // R
            pow(in_material.diffuse[1], kGamma),                                       // G
            pow(in_material.diffuse[2], kGamma),                                       // B
            in_material.diffuse_texname.empty()
                ? kInvalidTextureIndex
                : LoadTexture((path_to_folder + "/" + in_material.diffuse_texname).c_str()));

        out_material.specular_albedo = PackAlbedo(pow(in_material.specular[0], kGamma),  // R
            pow(in_material.specular[1], kGamma),                                        // G
            pow(in_material.specular[2], kGamma),                                        // B
            in_material.specular_texname.empty()
                ? kInvalidTextureIndex
                : LoadTexture((path_to_folder + "/" + in_material.specular_texname).c_str()));

        out_material.emission = PackRGBE(in_material.emission[0], in_material.emission[1], in_material.emission[2]);

        out_material.roughness_metalness = PackRoughnessMetalness(in_material.roughness,
            in_material.roughness_texname.empty()
                ? kInvalidTextureIndex
                : LoadTexture((path_to_folder + "/" + in_material.roughness_texname).c_str()),
            in_material.metallic,
            in_material.metallic_texname.empty()
                ? kInvalidTextureIndex
                : LoadTexture((path_to_folder + "/" + in_material.metallic_texname).c_str()));

        out_material.ior_emission_idx_transparency = PackIorEmissionIdxTransparency(in_material.ior,
            in_material.emissive_texname.empty()
                ? kInvalidTextureIndex
                : LoadTexture((path_to_folder + "/" + in_material.emissive_texname).c_str()),
            in_material.transmittance[0],
            in_material.alpha_texname.empty()
                ? kInvalidTextureIndex
                : LoadTexture((path_to_folder + "/" + in_material.alpha_texname).c_str()));
    }

    auto flip_vector = [](auto& vec, bool do_flip)
    {
        if (do_flip)
        {
            std::swap(vec.y, vec.z);
            vec.y = -vec.y;
        }
    };

    // Reserve memory (approx count) to reduce re-allocations
    size_t approx_triangles = 0;
    for (auto const& s : shapes)
        approx_triangles += s.mesh.indices.size() / 3;
    vertices_.reserve(vertices_.size() + approx_triangles * 3);
    indices_.reserve(indices_.size() + approx_triangles * 3);

    // Cache for vertex deduplication (position+normal+texcoord)
    std::unordered_map<Vertex, uint32_t, VertexHasher, VertexEqual> vertex_cache;

    for (auto const& shape : shapes)
    {
        auto const& indices = shape.mesh.indices;
        // The mesh is triangular
        assert(indices.size() % 3 == 0);

        for (uint32_t face = 0; face < indices.size() / 3; ++face)
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
            v1.position.x = attrib.vertices[pos_idx_1 * 3 + 0] * scale;
            v1.position.y = attrib.vertices[pos_idx_1 * 3 + 1] * scale;
            v1.position.z = attrib.vertices[pos_idx_1 * 3 + 2] * scale;

            v2.position.x = attrib.vertices[pos_idx_2 * 3 + 0] * scale;
            v2.position.y = attrib.vertices[pos_idx_2 * 3 + 1] * scale;
            v2.position.z = attrib.vertices[pos_idx_2 * 3 + 2] * scale;

            v3.position.x = attrib.vertices[pos_idx_3 * 3 + 0] * scale;
            v3.position.y = attrib.vertices[pos_idx_3 * 3 + 1] * scale;
            v3.position.z = attrib.vertices[pos_idx_3 * 3 + 2] * scale;

            auto compute_face_normal = [](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
            { return glm::normalize(glm::cross(b - a, c - a)); };
            bool has_n = (normal_idx_1 >= 0 && normal_idx_2 >= 0 && normal_idx_3 >= 0);

            if (has_n)
            {
                v1.normal.x = attrib.normals[normal_idx_1 * 3 + 0];
                v1.normal.y = attrib.normals[normal_idx_1 * 3 + 1];
                v1.normal.z = attrib.normals[normal_idx_1 * 3 + 2];

                v2.normal.x = attrib.normals[normal_idx_2 * 3 + 0];
                v2.normal.y = attrib.normals[normal_idx_2 * 3 + 1];
                v2.normal.z = attrib.normals[normal_idx_2 * 3 + 2];

                v3.normal.x = attrib.normals[normal_idx_3 * 3 + 0];
                v3.normal.y = attrib.normals[normal_idx_3 * 3 + 1];
                v3.normal.z = attrib.normals[normal_idx_3 * 3 + 2];
            }
            else
            {
                glm::vec3 n = compute_face_normal(glm::vec3(v1.position),
                    glm::vec3(v2.position),
                    glm::vec3(v3.position));
                v1.normal = glm::vec4(n, v1.normal.w);
                v2.normal = glm::vec4(n, v2.normal.w);
                v3.normal = glm::vec4(n, v3.normal.w);
            }

            v1.texcoord.x = texcoord_idx_1 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_1 * 2 + 0];
            v1.texcoord.y = texcoord_idx_1 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_1 * 2 + 1];

            v2.texcoord.x = texcoord_idx_2 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_2 * 2 + 0];
            v2.texcoord.y = texcoord_idx_2 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_2 * 2 + 1];

            v3.texcoord.x = texcoord_idx_3 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_3 * 2 + 0];
            v3.texcoord.y = texcoord_idx_3 < 0 ? 0.0f : attrib.texcoords[texcoord_idx_3 * 2 + 1];

            flip_vector(v1.position, flip_yz);
            flip_vector(v1.normal, flip_yz);
            flip_vector(v2.position, flip_yz);
            flip_vector(v2.normal, flip_yz);
            flip_vector(v3.position, flip_yz);
            flip_vector(v3.normal, flip_yz);

            // Vertex deduplication: reuse vertices with identical attributes
            auto find_or_add = [&](Vertex const& v) -> uint32_t
            {
                auto it = vertex_cache.find(v);
                if (it != vertex_cache.end())
                    return it->second;
                uint32_t idx = static_cast<uint32_t>(vertices_.size());
                vertices_.push_back(v);
                vertex_cache.emplace(v, idx);
                return idx;
            };

            uint32_t i1 = find_or_add(v1);
            uint32_t i2 = find_or_add(v2);
            uint32_t i3 = find_or_add(v3);

            indices_.push_back(i1);
            indices_.push_back(i2);
            indices_.push_back(i3);

            if (shape.mesh.material_ids[face] >= 0 && shape.mesh.material_ids[face] < materials_.size())
            {
                triangle_material_indices_.push_back(shape.mesh.material_ids[face]);
            }
            else
            {
                // Use the default material
                triangle_material_indices_.push_back(0);
            }
        }
    }

    std::cout << "Load successful (" << indices_.size() / 3 << " triangles)" << std::endl;
}

size_t Scene::LoadTexture(char const* filename)
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
    else if (strcmp(file_extension, ".jpg") == 0 || strcmp(file_extension, ".tga") == 0
        || strcmp(file_extension, ".png") == 0)
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
    texture.data_start = (uint32_t)texture_data_.size();

    size_t texture_idx = textures_.size();
    textures_.push_back(std::move(texture));

    texture_data_.insert(texture_data_.end(), image.data.begin(), image.data.end());

    // Cache the texture
    loaded_textures_.emplace(filename, texture_idx);
    return texture_idx;
}

void Scene::CollectEmissiveTriangles()
{
    for (auto triangle_idx = 0; triangle_idx < triangle_material_indices_.size(); ++triangle_idx)
    {
        uint32_t material_index = triangle_material_indices_[triangle_idx];
        glm::vec3 emission = UnpackRGBE(materials_[material_index].emission);

        if (emission.x + emission.y + emission.z > 0.0f)
        {
            // The triangle is emissive
            emissive_indices_.push_back(triangle_idx);
        }
    }

    scene_info_.emissive_count = (uint32_t)emissive_indices_.size();
}

void Scene::AddPointLight(glm::vec3 origin, glm::vec3 radiance)
{
    Light light = {};
    light.origin = glm::vec4(origin.x, origin.y, origin.z, 0.0f);
    light.radiance = glm::vec4(radiance.x, radiance.y, radiance.z, 0.0f);
    light.type = LIGHT_TYPE_POINT;
    lights_.push_back(std::move(light));
}

void Scene::AddDirectionalLight(glm::vec3 direction, glm::vec3 radiance)
{
    direction = glm::normalize(direction);

    Light light = {};
    light.origin = glm::vec4(direction.x, direction.y, direction.z, 0.0f);
    light.radiance = glm::vec4(radiance.x, radiance.y, radiance.z, 0.0f);
    light.type = LIGHT_TYPE_DIRECTIONAL;
    lights_.emplace_back(std::move(light));
}

void Scene::Finalize()
{
    CollectEmissiveTriangles();

    // scene_info_.environment_map_index = LoadTexture("textures/studio_small_03_4k.hdr");
    scene_info_.analytic_light_count = (uint32_t)lights_.size();

    LoadHDR("assets/ibl/CGSkies_0036_free.hdr", env_image_);
}
