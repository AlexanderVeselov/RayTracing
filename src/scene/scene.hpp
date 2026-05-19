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

#pragma once

#include "managers/texture_manager.hpp"
#include "shaders/shared_structures.h"

#include <string>
#include <vector>

class Scene
{
public:
    Scene(const char* filename, float scale, bool flip_yz, TextureManager& texture_manager);

    std::vector<Vertex> const& GetVertices() const { return vertices_; }
    std::vector<uint32_t> const& GetIndices() const { return indices_; }
    std::vector<uint32_t> const& GetTriangleMaterialIndices() const { return triangle_material_indices_; }
    std::vector<uint32_t> const& GetEmissiveIndices() const { return emissive_indices_; }
    std::vector<PackedMaterial> const& GetMaterials() const { return materials_; }
    std::vector<Light> const& GetLights() const { return lights_; }
    SceneInfo const& GetSceneInfo() const { return scene_info_; }
    void Finalize();
    void AddPointLight(glm::vec3 origin, glm::vec3 radiance);
    void AddDirectionalLight(glm::vec3 direction, glm::vec3 radiance);

private:
    void Load(char const* filename, float scale, bool flip_yz);
    // Returns texture index in TextureManager.
    uint32_t LoadTexture(char const* filename);
    void CollectEmissiveTriangles();

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<uint32_t> triangle_material_indices_;
    std::vector<uint32_t> emissive_indices_;
    std::vector<PackedMaterial> materials_;
    std::vector<Light> lights_;
    SceneInfo scene_info_ = {};
    TextureManager& texture_manager_;
};
