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

#include "acceleration_structure.hpp"
#include "utils/aabb.hpp"

#include <memory>

class Bvh : public AccelerationStructure
{
public:
    Bvh() = default;

    void BuildCPU(std::vector<Vertex> const& vertices, std::vector<uint32_t> const& indices,
        std::vector<MeshInfo> const& meshes, std::vector<InstanceInfo> const& instances) override;
    std::vector<LinearBVHNode> const& GetNodes() const override { return nodes_; }
    std::vector<RTTriangle> const& GetTriangles() const override { return rt_triangles_; }

    struct BVHPrimitiveInfo
    {
        unsigned int primitiveNumber;
        unsigned int instanceIndex;
        Aabb bounds;
        glm::vec3 centroid;
    };

    struct BVHBuildNode
    {
        void InitLeaf(int first, int n, const Aabb& b)
        {
            firstPrimOffset = first;
            nPrimitives = n;
            bounds = b;
            children[0] = children[1] = nullptr;
        }

        void InitInterior(int axis, BVHBuildNode* c0, BVHBuildNode* c1)
        {
            children[0] = c0;
            children[1] = c1;
            bounds = Union(c0->bounds, c1->bounds);
            splitAxis = axis;
            nPrimitives = 0;
        }

        Aabb bounds;
        BVHBuildNode* children[2];
        int splitAxis, firstPrimOffset, nPrimitives;
    };

    struct BucketInfo
    {
        int count = 0;
        Aabb bounds;
    };

private:
    BVHBuildNode* RecursiveBuild(std::vector<Vertex> const& vertices, std::vector<uint32_t> const& src_indices,
        std::vector<MeshInfo> const& meshes, std::vector<InstanceInfo> const& instances,
        std::vector<BVHPrimitiveInfo>& primitiveInfo, unsigned int start, unsigned int end, unsigned int* totalNodes,
        std::vector<RTTriangle>& orderedTris);

    unsigned int FlattenBVHTree(BVHBuildNode* node, unsigned int* offset);

    std::vector<LinearBVHNode> nodes_;
    std::vector<RTTriangle> rt_triangles_;
    BVHBuildNode* root_node_;
    uint32_t max_prims_in_node_;
};
