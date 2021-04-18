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
    cl_mem GetMaterialBuffer() const { return material_buffer_(); }
    cl_mem GetEnvTextureBuffer() const { return env_texture_(); }
    void UploadBuffers();

private:
    void LoadTriangles(const char* filename);

    CLContext& cl_context_;
    std::vector<std::string> m_MaterialNames;
    std::vector<Triangle> m_Triangles;
    std::vector<Material> m_Materials;
    cl::Buffer triangle_buffer_;
    cl::Buffer material_buffer_;
    cl::Image2D env_texture_;

};

#endif // SCENE_HPP
