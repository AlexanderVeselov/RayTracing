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

#include <glm/glm.hpp>

#include <vector>

class ObjLoader;

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t material_index = 0;
};

struct SceneInstance
{
    uint32_t mesh_index = 0;
    glm::mat4 transform = glm::mat4(1.0f);
    glm::mat4 inverse_transform = glm::mat4(1.0f);
    glm::mat3 normal_transform = glm::mat3(1.0f);
};

class Scene
{
public:
    Scene(const char* filename, float scale, bool flip_yz, TextureManager& texture_manager);

    uint32_t AddInstance(uint32_t mesh_index, glm::mat4 const& transform);
    void RebuildGeometryBuffers();

    std::vector<Mesh> const& GetMeshes() const { return meshes_; }
    std::vector<SceneInstance> const& GetInstances() const { return instances_; }
    std::vector<Vertex> const& GetVertices() const { return vertices_; }
    std::vector<uint32_t> const& GetIndices() const { return indices_; }
    std::vector<MeshInfo> const& GetMeshInfos() const { return mesh_infos_; }
    std::vector<InstanceInfo> const& GetInstanceInfos() const { return instance_infos_; }
    std::vector<uint32_t> const& GetEmissiveIndices() const { return emissive_indices_; }
    std::vector<PackedMaterial> const& GetMaterials() const { return materials_; }
    std::vector<Light> const& GetLights() const { return lights_; }
    SceneInfo const& GetSceneInfo() const { return scene_info_; }
    void Finalize();
    void AddPointLight(glm::vec3 origin, glm::vec3 radiance);
    void AddDirectionalLight(glm::vec3 direction, glm::vec3 radiance);

private:
    friend class ObjLoader;

    void CollectEmissiveTriangles();

    std::vector<Mesh> meshes_;
    std::vector<SceneInstance> instances_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<MeshInfo> mesh_infos_;
    std::vector<InstanceInfo> instance_infos_;
    std::vector<uint32_t> emissive_indices_;
    std::vector<PackedMaterial> materials_;
    std::vector<Light> lights_;
    SceneInfo scene_info_ = {};
    TextureManager& texture_manager_;
};
