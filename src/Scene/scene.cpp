#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "scene.hpp"
#include "mathlib/mathlib.hpp"
#include "render.hpp"
#include "utils/cl_exception.hpp"
#include "io/image_loader.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include <cctype>
#include <GL/glew.h>

Scene::Scene(const char* filename, CLContext& cl_context)
    : cl_context_(cl_context)
{
    LoadTriangles(filename);
}

std::vector<Triangle>& Scene::GetTriangles()
{
    return m_Triangles;
}

//void ComputeTangentSpace(Vertex& v1, Vertex& v2, Vertex& v3)
//{
//    const float3& v1p = v1.position;
//    const float3& v2p = v2.position;
//    const float3& v3p = v3.position;
//
//    const float2& v1t = v1.texcoord;
//    const float2& v2t = v2.texcoord;
//    const float2& v3t = v3.texcoord;
//
//    double x1 = v2p.x - v1p.x;
//    double x2 = v3p.x - v1p.x;
//    double y1 = v2p.y - v1p.y;
//    double y2 = v3p.y - v1p.y;
//    double z1 = v2p.z - v1p.z;
//    double z2 = v3p.z - v1p.z;
//
//    double s1 = v2t.x - v1t.x;
//    double s2 = v3t.x - v1t.x;
//    double t1 = v2t.y - v1t.y;
//    double t2 = v3t.y - v1t.y;
//
//    double r = 1.0 / (s1 * t2 - s2 * t1);
//    float3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
//    float3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
//
//    v1.tangent_s += sdir;
//    v2.tangent_s += sdir;
//    v3.tangent_s += sdir;
//
//    v1.tangent_t += tdir;
//    v2.tangent_t += tdir;
//    v3.tangent_t += tdir;
//
//}

void Scene::LoadTriangles(const char* filename)
{
    std::vector<float3> positions;
    std::vector<float3> normals;
    std::vector<float2> texcoords;

    Material currentMaterial;

    std::cout << "Loading object file " << filename << std::endl;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, "meshes/");

    if (!success)
    {
        throw std::runtime_error("Failed to load scene!");
    }

    m_Materials.resize(materials.size());

    for (std::uint32_t material_idx = 0; material_idx < materials.size(); ++material_idx)
    {
        auto& out_material = m_Materials[material_idx];
        auto const& in_material = materials[material_idx];

        out_material.diffuse_albedo.x = in_material.diffuse[0];
        out_material.diffuse_albedo.y = in_material.diffuse[1];
        out_material.diffuse_albedo.z = in_material.diffuse[2];

        out_material.specular_albedo.x = in_material.specular[0];
        out_material.specular_albedo.y = in_material.specular[1];
        out_material.specular_albedo.z = in_material.specular[2];

        out_material.emission.x = in_material.emission[0];
        out_material.emission.y = in_material.emission[1];
        out_material.emission.z = in_material.emission[2];

        out_material.roughness = in_material.roughness;

        out_material.metalness = in_material.metallic;

        out_material.ior = in_material.ior;
    }

    for (auto const& shape : shapes)
    {
        auto const& indices = shape.mesh.indices;
        // The mesh is triangular
        assert(indices.size() % 3 == 0);

        for (std::uint32_t face = 0; face < indices.size() / 3; ++face)
        {
            auto pos_idx_1 = indices[face * 3 + 0].vertex_index;
            auto pos_idx_2 = indices[face * 3 + 1].vertex_index;
            auto pos_idx_3 = indices[face * 3 + 2].vertex_index;

            auto normal_idx_1 = indices[face * 3 + 0].normal_index;
            auto normal_idx_2 = indices[face * 3 + 1].normal_index;
            auto normal_idx_3 = indices[face * 3 + 2].normal_index;

            Vertex v1;
            v1.position.x = attrib.vertices[pos_idx_1 * 3 + 0];
            v1.position.y = attrib.vertices[pos_idx_1 * 3 + 1];
            v1.position.z = attrib.vertices[pos_idx_1 * 3 + 2];

            v1.normal.x = attrib.normals[normal_idx_1 * 3 + 0];
            v1.normal.y = attrib.normals[normal_idx_1 * 3 + 1];
            v1.normal.z = attrib.normals[normal_idx_1 * 3 + 2];

            Vertex v2;
            v2.position.x = attrib.vertices[pos_idx_2 * 3 + 0];
            v2.position.y = attrib.vertices[pos_idx_2 * 3 + 1];
            v2.position.z = attrib.vertices[pos_idx_2 * 3 + 2];

            v2.normal.x = attrib.normals[normal_idx_2 * 3 + 0];
            v2.normal.y = attrib.normals[normal_idx_2 * 3 + 1];
            v2.normal.z = attrib.normals[normal_idx_2 * 3 + 2];

            Vertex v3;
            v3.position.x = attrib.vertices[pos_idx_3 * 3 + 0];
            v3.position.y = attrib.vertices[pos_idx_3 * 3 + 1];
            v3.position.z = attrib.vertices[pos_idx_3 * 3 + 2];

            v3.normal.x = attrib.normals[normal_idx_3 * 3 + 0];
            v3.normal.y = attrib.normals[normal_idx_3 * 3 + 1];
            v3.normal.z = attrib.normals[normal_idx_3 * 3 + 2];

            assert(shape.mesh.material_ids[face] >= 0
                && shape.mesh.material_ids[face] < m_Materials.size());
            m_Triangles.emplace_back(v1, v2, v3, shape.mesh.material_ids[face]);
        }
    }

    std::cout << "Load successful (" << m_Triangles.size() << " triangles)" << std::endl;

}

void Scene::UploadBuffers()
{
    cl_int status;

    triangle_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m_Triangles.size() * sizeof(Triangle), m_Triangles.data(), &status);
    if (status != CL_SUCCESS)
    {
        throw CLException("Failed to create scene buffer", status);
    }

    material_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m_Materials.size() * sizeof(Material), m_Materials.data(), &status);
    if (status != CL_SUCCESS)
    {
        throw CLException("Failed to create material buffer", status);
    }

    ///@TODO: remove from here
    // Texture Buffers
    cl::ImageFormat image_format;
    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_FLOAT;

    Image image;
    HDRLoader::Load("textures/Topanga_Forest_B_3k.hdr", image);
    env_texture_ = cl::Image2D(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        image_format, image.width, image.height, 0, image.colors, &status);

    if (status != CL_SUCCESS)
    {
        throw CLException("Failed to create environment image", status);
    }

}
