#ifndef SCENE_HPP
#define SCENE_HPP

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
    void LoadTriangles(const char* filename);
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

#endif // SCENE_HPP
