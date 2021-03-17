#ifndef BVH_HPP
#define BVH_HPP

#include "acceleration_structure.hpp"

class CLContext;
class Bvh : public AccelerationStructure
{
public:
    Bvh(CLContext& cl_context) : AccelerationStructure(cl_context) {}
    void BuildCPU(std::vector<Triangle> const& triangles) override;
    void IntersectRays(cl::CommandQueue const& queue) override;

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
        std::vector<BVHPrimitiveInfo>& primitiveInfo,
        unsigned int start,
        unsigned int end, unsigned int* totalNodes,
        std::vector<Triangle>& orderedTriangles);
    unsigned int FlattenBVHTree(BVHBuildNode* node, unsigned int* offset);

    std::uint32_t max_prims_in_node_;
    cl::Buffer node_buffer_;
};

#endif // BVH_HPP
