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
#include <CL/cl.hpp>
#include <vector>

class CLContext;
class Scene
{
public:
    Scene(const char* filename, CLContext& cl_context);
    // TODO: REPLACE TO CONST REF
    std::vector<Triangle> & GetTriangles();
    cl_mem GetTriangleBuffer() const { return triangle_buffer_(); }
    cl_mem GetEmissiveIndicesBuffer() const { return emissive_buffer_(); }
    cl_mem GetMaterialBuffer() const { return material_buffer_(); }
    cl_mem GetEnvTextureBuffer() const { return env_texture_(); }
    cl_mem GetAnalyticLightBuffer() const { return analytic_light_buffer_(); }
    SceneInfo const& GetSceneInfo() const { return scene_info_; }
    void Finalize();
    void AddPointLight(float3 origin, float3 radiance);
    void AddDirectionalLight(float3 direction, float3 radiance);

private:
    void Load(const char* filename);
    void CollectEmissiveTriangles();

    CLContext& cl_context_;
    std::vector<Triangle> triangles_;
    std::vector<std::uint32_t> emissive_indices_;
    std::vector<Material> materials_;
    std::vector<Light> lights_;
    SceneInfo scene_info_ = {};

    cl::Buffer triangle_buffer_;
    cl::Buffer material_buffer_;
    cl::Buffer emissive_buffer_;
    cl::Buffer analytic_light_buffer_;
    cl::Buffer scene_info_buffer_;
    cl::Image2D env_texture_;

};
