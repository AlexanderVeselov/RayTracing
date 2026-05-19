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

#include "obj_loader.hpp"

#include "managers/texture_manager.hpp"
#include "scene/scene.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#undef max

namespace
{
// Simple FNV-1a hash combiner for 32-bit values.
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

    int e;
    v = frexp(v, &e) * 256.0f / v;
    return ((unsigned int)(r * v)) | ((unsigned int)(g * v) << 8) | ((unsigned int)(b * v) << 16) | ((e + 128) << 24);
}

unsigned int PackRoughnessMetalness(float roughness, uint32_t roughness_idx, float metalness, uint32_t metalness_idx)
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

void ObjLoader::Load(Scene& scene, char const* filename, float scale, bool flip_yz, TextureManager& texture_manager)
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

    scene.materials_.resize(materials.size());

    const float kGamma = 2.2f;
    const uint32_t kInvalidTextureIndex = 0xFF;

    for (uint32_t material_idx = 0; material_idx < materials.size(); ++material_idx)
    {
        auto& out_material = scene.materials_[material_idx];
        auto const& in_material = materials[material_idx];

        out_material.diffuse_albedo = PackAlbedo(pow(in_material.diffuse[0], kGamma),
            pow(in_material.diffuse[1], kGamma),
            pow(in_material.diffuse[2], kGamma),
            in_material.diffuse_texname.empty()
                ? kInvalidTextureIndex
                : texture_manager.LoadTexture(path_to_folder + "/" + in_material.diffuse_texname));

        out_material.specular_albedo = PackAlbedo(pow(in_material.specular[0], kGamma),
            pow(in_material.specular[1], kGamma),
            pow(in_material.specular[2], kGamma),
            in_material.specular_texname.empty()
                ? kInvalidTextureIndex
                : texture_manager.LoadTexture(path_to_folder + "/" + in_material.specular_texname));

        out_material.emission = PackRGBE(in_material.emission[0], in_material.emission[1], in_material.emission[2]);

        out_material.roughness_metalness = PackRoughnessMetalness(in_material.roughness,
            in_material.roughness_texname.empty()
                ? kInvalidTextureIndex
                : texture_manager.LoadTexture(path_to_folder + "/" + in_material.roughness_texname),
            in_material.metallic,
            in_material.metallic_texname.empty()
                ? kInvalidTextureIndex
                : texture_manager.LoadTexture(path_to_folder + "/" + in_material.metallic_texname));

        out_material.ior_emission_idx_transparency = PackIorEmissionIdxTransparency(in_material.ior,
            in_material.emissive_texname.empty()
                ? kInvalidTextureIndex
                : texture_manager.LoadTexture(path_to_folder + "/" + in_material.emissive_texname),
            in_material.transmittance[0],
            in_material.alpha_texname.empty()
                ? kInvalidTextureIndex
                : texture_manager.LoadTexture(path_to_folder + "/" + in_material.alpha_texname));
    }

    auto flip_vector = [](auto& vec, bool do_flip)
    {
        if (do_flip)
        {
            std::swap(vec.y, vec.z);
            vec.y = -vec.y;
        }
    };

    size_t approx_triangles = 0;
    for (auto const& shape : shapes)
    {
        approx_triangles += shape.mesh.indices.size() / 3;
    }
    scene.vertices_.reserve(scene.vertices_.size() + approx_triangles * 3);
    scene.indices_.reserve(scene.indices_.size() + approx_triangles * 3);

    std::unordered_map<Vertex, uint32_t, VertexHasher, VertexEqual> vertex_cache;

    for (auto const& shape : shapes)
    {
        auto const& indices = shape.mesh.indices;
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
                glm::vec3 n = compute_face_normal(v1.position, v2.position, v3.position);
                v1.normal = n;
                v2.normal = n;
                v3.normal = n;
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

            auto find_or_add = [&](Vertex const& v) -> uint32_t
            {
                auto it = vertex_cache.find(v);
                if (it != vertex_cache.end())
                    return it->second;
                uint32_t idx = static_cast<uint32_t>(scene.vertices_.size());
                scene.vertices_.push_back(v);
                vertex_cache.emplace(v, idx);
                return idx;
            };

            uint32_t i1 = find_or_add(v1);
            uint32_t i2 = find_or_add(v2);
            uint32_t i3 = find_or_add(v3);

            scene.indices_.push_back(i1);
            scene.indices_.push_back(i2);
            scene.indices_.push_back(i3);

            if (shape.mesh.material_ids[face] >= 0 && shape.mesh.material_ids[face] < scene.materials_.size())
            {
                scene.triangle_material_indices_.push_back(shape.mesh.material_ids[face]);
            }
            else
            {
                scene.triangle_material_indices_.push_back(0);
            }
        }
    }

    std::cout << "Load successful (" << scene.indices_.size() / 3 << " triangles)" << std::endl;
}
