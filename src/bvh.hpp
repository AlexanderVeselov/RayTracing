#ifndef BVH_HPP
#define BVH_HPP

#include "acceleration_structure.hpp"
#include <memory>

class CLContext;
class CLKernel;
class Bvh : public AccelerationStructure
{
public:
    Bvh(CLContext& cl_context);

    // TODO: USE CONSTANT REF
    void BuildCPU(std::vector<Triangle> & triangles) override;
    void IntersectRays(cl::CommandQueue const& queue,
        cl::Buffer const& rays_buffer, cl::Buffer const& hits_buffer) override;

    struct BVHPrimitiveInfo
    {
        BVHPrimitiveInfo() {}
        BVHPrimitiveInfo(unsigned int primitiveNumber, const Bounds3& bounds)
            : primitiveNumber(primitiveNumber), bounds(bounds),
            centroid(bounds.min * 0.5f + bounds.max * 0.5f)
        {}

        unsigned int primitiveNumber;
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
    cl::Buffer node_buffer_;
    std::unique_ptr<CLKernel> intersect_kernel_;
};

#endif // BVH_HPP
