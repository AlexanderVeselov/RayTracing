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
    RebuildFlattenedGeometry();
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

uint32_t Scene::AddInstance(uint32_t model_index, glm::mat4 const& transform)
{
    if (model_index >= models_.size())
    {
        throw std::runtime_error("Scene::AddInstance: model index is out of range");
    }

    SceneInstance instance = {};
    instance.model_index = model_index;
    instance.transform = transform;
    instance.inverse_transform = glm::inverse(transform);
    instance.normal_transform = glm::inverseTranspose(glm::mat3(transform));

    instances_.push_back(instance);
    return static_cast<uint32_t>(instances_.size() - 1);
}

void Scene::RebuildFlattenedGeometry()
{
    vertices_.clear();
    indices_.clear();
    triangle_material_indices_.clear();
    emissive_indices_.clear();

    size_t vertex_count = 0;
    size_t index_count = 0;
    for (SceneInstance const& instance : instances_)
    {
        Model const& model = models_[instance.model_index];
        for (uint32_t mesh_index : model.mesh_indices)
        {
            Mesh const& mesh = meshes_[mesh_index];
            vertex_count += mesh.vertices.size();
            index_count += mesh.indices.size();
        }
    }

    vertices_.reserve(vertex_count);
    indices_.reserve(index_count);
    triangle_material_indices_.reserve(index_count / 3);

    for (SceneInstance const& instance : instances_)
    {
        Model const& model = models_[instance.model_index];
        for (uint32_t mesh_index : model.mesh_indices)
        {
            Mesh const& mesh = meshes_[mesh_index];
            uint32_t vertex_offset = static_cast<uint32_t>(vertices_.size());

            for (Vertex vertex : mesh.vertices)
            {
                vertex.position = glm::vec3(instance.transform * glm::vec4(vertex.position, 1.0f));
                vertex.normal = glm::normalize(instance.normal_transform * vertex.normal);
                vertices_.push_back(vertex);
            }

            for (uint32_t index : mesh.indices)
            {
                indices_.push_back(vertex_offset + index);
            }

            uint32_t triangle_count = static_cast<uint32_t>(mesh.indices.size() / 3);
            for (uint32_t triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
            {
                triangle_material_indices_.push_back(mesh.material_index);
            }
        }
    }
}

void Scene::CollectEmissiveTriangles()
{
    for (auto triangle_idx = 0; triangle_idx < triangle_material_indices_.size(); ++triangle_idx)
    {
        uint32_t material_index = triangle_material_indices_[triangle_idx];
        glm::vec3 emission = UnpackRGBE(materials_[material_index].emission);

        if (emission.x + emission.y + emission.z > 0.0f)
        {
            emissive_indices_.push_back(triangle_idx);
        }
    }

    scene_info_.emissive_count = (uint32_t)emissive_indices_.size();
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

    scene_info_.analytic_light_count = (uint32_t)lights_.size();
    scene_info_.environment_map_index = texture_manager_.LoadTexture("assets/ibl/CGSkies_0036_free.hdr");
}
