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

#pragma once

#include "mathlib/mathlib.hpp"
#include "kernels/shared_structures.h"
#include "io/image_loader.hpp"
#include <vector>
#include <unordered_map>

class CLContext;
class Scene
{
public:
    Scene(const char* filename, float scale, bool flip_yz);

    std::vector<Triangle>& GetTriangles() { return triangles_; }
    std::vector<Triangle> const& GetTriangles() const { return triangles_; }
    std::vector<std::uint32_t> const& GetEmissiveIndices() const { return emissive_indices_; }
    std::vector<PackedMaterial> const& GetMaterials() const { return materials_; }
    std::vector<Texture> const& GetTextures() const { return textures_; }
    std::vector<std::uint32_t> const& GetTextureData() const { return texture_data_; }
    std::vector<Light> const& GetLights() const { return lights_; }
    SceneInfo const& GetSceneInfo() const { return scene_info_; }
    Image const& GetEnvImage() const { return env_image_; }
    void Finalize();
    void AddPointLight(float3 origin, float3 radiance);
    void AddDirectionalLight(float3 direction, float3 radiance);

private:
    void Load(char const* filename, float scale, bool flip_yz);
    // Returns texture index in textures_
    std::size_t LoadTexture(char const* filename);
    void CollectEmissiveTriangles();

    std::vector<Triangle> triangles_;
    std::vector<std::uint32_t> emissive_indices_;
    std::vector<PackedMaterial> materials_;
    std::vector<Light> lights_;
    std::vector<Texture> textures_;
    std::vector<std::uint32_t> texture_data_;
    std::unordered_map<std::string, std::size_t> loaded_textures_;
    SceneInfo scene_info_ = {};
    Image env_image_;
};
