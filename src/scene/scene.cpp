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

#include "scene.hpp"

#include "loaders/obj_loader.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <cmath>
#include <stdexcept>
#include <utility>

Scene::Scene(const char* filename, float scale, bool flip_yz, TextureManager& texture_manager)
    : texture_manager_(texture_manager)
{
    ObjLoader::Load(*this, filename, scale, flip_yz, texture_manager_);
    RebuildGeometryBuffers();
}

namespace
{
glm::vec3 UnpackRGBE(unsigned int rgbe)
{
    float f;
    int r = (rgbe >> 0) & 0xFF;
    int g = (rgbe >> 8) & 0xFF;
    int b = (rgbe >> 16) & 0xFF;
    int exp = rgbe >> 24;

    if (exp)
    {
        f = ldexp(1.0f, exp - (int)(128 + 8));
        return glm::vec3((float)r, (float)g, (float)b) * f;
    }

    return glm::vec3(0.0f);
}
}  // namespace

uint32_t Scene::AddInstance(uint32_t mesh_index, glm::mat4 const& transform)
{
    if (mesh_index >= meshes_.size())
    {
        throw std::runtime_error("Scene::AddInstance: mesh index is out of range");
    }

    SceneInstance instance = {};
    instance.mesh_index = mesh_index;
    instance.transform = transform;
    instance.inverse_transform = glm::inverse(transform);
    instance.normal_transform = glm::inverseTranspose(glm::mat3(transform));

    instances_.push_back(instance);
    return static_cast<uint32_t>(instances_.size() - 1);
}

void Scene::RebuildGeometryBuffers()
{
    vertices_.clear();
    indices_.clear();
    mesh_infos_.clear();
    instance_infos_.clear();
    emissive_triangles_.clear();

    size_t vertex_count = 0;
    size_t index_count = 0;
    for (Mesh const& mesh : meshes_)
    {
        vertex_count += mesh.vertices.size();
        index_count += mesh.indices.size();
    }

    vertices_.reserve(vertex_count);
    indices_.reserve(index_count);
    mesh_infos_.reserve(meshes_.size());
    instance_infos_.reserve(instances_.size());

    for (Mesh const& mesh : meshes_)
    {
        MeshInfo mesh_info = {};
        mesh_info.vertex_offset = static_cast<uint32_t>(vertices_.size());
        mesh_info.index_offset = static_cast<uint32_t>(indices_.size());
        mesh_info.triangle_count = static_cast<uint32_t>(mesh.indices.size() / 3);
        mesh_info.material_index = mesh.material_index;
        mesh_infos_.push_back(mesh_info);

        vertices_.insert(vertices_.end(), mesh.vertices.begin(), mesh.vertices.end());
        indices_.insert(indices_.end(), mesh.indices.begin(), mesh.indices.end());
    }

    for (SceneInstance const& instance : instances_)
    {
        InstanceInfo instance_info = {};
        instance_info.mesh_index = instance.mesh_index;
        instance_info.transform = instance.transform;
        instance_info.normal_transform = glm::mat4(instance.normal_transform);
        instance_infos_.push_back(instance_info);
    }
}

void Scene::CollectEmissiveTriangles()
{
    for (uint32_t instance_index = 0; instance_index < instances_.size(); ++instance_index)
    {
        SceneInstance const& instance = instances_[instance_index];
        Mesh const& mesh = meshes_[instance.mesh_index];
        glm::vec3 emission = UnpackRGBE(materials_[mesh.material_index].emission);
        if (emission.x + emission.y + emission.z <= 0.0f)
        {
            continue;
        }

        uint32_t triangle_count = static_cast<uint32_t>(mesh.indices.size() / 3);
        for (uint32_t triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
        {
            emissive_triangles_.push_back({ instance_index, triangle_index });
        }
    }
}

void Scene::AddPointLight(glm::vec3 origin, glm::vec3 radiance)
{
    Light light = {};
    light.origin = origin;
    light.radiance = radiance;
    light.type = LIGHT_TYPE_POINT;
    lights_.push_back(std::move(light));
}

void Scene::AddDirectionalLight(glm::vec3 direction, glm::vec3 radiance)
{
    direction = glm::normalize(direction);

    Light light = {};
    light.origin = direction;
    light.radiance = radiance;
    light.type = LIGHT_TYPE_DIRECTIONAL;
    lights_.emplace_back(std::move(light));
}

void Scene::Finalize()
{
    CollectEmissiveTriangles();

    environment_map_index_ = texture_manager_.LoadTexture("assets/ibl/CGSkies_0036_free.hdr");
}
