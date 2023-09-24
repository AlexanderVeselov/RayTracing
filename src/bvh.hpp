/*****************************************************************************
 MIT License

 Copyright(c) 2023 Alexander Veselov

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
#include <memory>

class Bvh : public AccelerationStructure
{
public:
    Bvh();

    // TODO: USE CONSTANT REF
    void BuildCPU(std::vector<Triangle> & triangles) override;
    std::vector<LinearBVHNode> const& GetNodes() const override { return nodes_; }

    //void IntersectRays(cl::Buffer const& rays_buffer, cl::Buffer const& ray_counter_buffer,
    //    std::uint32_t max_num_rays, cl::Buffer const& hits_buffer, bool closest_hit = true) override;



    struct BVHPrimitiveInfo
    {
        BVHPrimitiveInfo() {}
        BVHPrimitiveInfo(unsigned int primitiveNumber, const Bounds3& bounds)
            : primitiveNumber(primitiveNumber), bounds(bounds),
            centroid(bounds.min * 0.5f + bounds.max * 0.5f)
        {}

        unsigned int primitiveNumber = 0;
        Bounds3 bounds;
        float3 centroid;

    };

    struct BVHBuildNode
    {
        void InitLeaf(int first, int n, const Bounds3& b)
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

        Bounds3 bounds;
        BVHBuildNode* children[2];
        int splitAxis, firstPrimOffset, nPrimitives;

    };

    struct BucketInfo
    {
        int count = 0;
        Bounds3 bounds;
    };

private:
    BVHBuildNode* RecursiveBuild(
        std::vector<Triangle> const& triangles,
        std::vector<BVHPrimitiveInfo>& primitiveInfo,
        unsigned int start,
        unsigned int end, unsigned int* totalNodes,
        std::vector<Triangle>& orderedTriangles);
    unsigned int FlattenBVHTree(BVHBuildNode* node, unsigned int* offset);

    std::vector<LinearBVHNode> nodes_;
    BVHBuildNode* root_node_;
    std::uint32_t max_prims_in_node_;
};
