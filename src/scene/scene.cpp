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

#include <cmath>
#include <utility>

Scene::Scene(const char* filename, float scale, bool flip_yz, TextureManager& texture_manager)
    : texture_manager_(texture_manager)
{
    ObjLoader::Load(*this, filename, scale, flip_yz, texture_manager_);
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
