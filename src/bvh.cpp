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

#include "bvh.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
constexpr auto kMaxPrimitivesInNode = 4u;
}

void Bvh::BuildCPU(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
{
    std::cout << "Building BVH (VB/IB -> compact tri buffer) ..." << std::endl;

    if (indices.size() % 3 != 0)
        throw std::runtime_error("Indices count must be divisible by 3");

    const unsigned triangle_count = static_cast<unsigned>(indices.size() / 3);

    // 1. Collect primitives: AABB and centroids based on VB/IB
    std::vector<BVHPrimitiveInfo> prim_info(triangle_count);
    prim_info.reserve(triangle_count);

    for (unsigned t = 0; t < triangle_count; ++t)
    {
        const uint32_t i0 = indices[3 * t + 0];
        const uint32_t i1 = indices[3 * t + 1];
        const uint32_t i2 = indices[3 * t + 2];

        const glm::vec3 p0 = glm::vec3(vertices[i0].position);
        const glm::vec3 p1 = glm::vec3(vertices[i1].position);
        const glm::vec3 p2 = glm::vec3(vertices[i2].position);

        Aabb tri_aabb;
        tri_aabb = Union(tri_aabb, p0);
        tri_aabb = Union(tri_aabb, p1);
        tri_aabb = Union(tri_aabb, p2);
        const glm::vec3 c = (p0 + p1 + p2) * (1.0f / 3.0f);

        prim_info[t] = {t, tri_aabb, c};
    }

    // 2. Recursively build
    unsigned total_nodes = 0;
    rt_triangles_.reserve(triangle_count);
    root_node_ = RecursiveBuild(vertices, indices, prim_info, 0, triangle_count, &total_nodes, rt_triangles_);

    // 3. Flatten
    nodes_.resize(total_nodes);
    unsigned off = 0;
    FlattenBVHTree(root_node_, &off);
    assert(off == total_nodes);

    std::cout << "BVH nodes: " << total_nodes << ", tris in buffer: " << rt_triangles_.size() << " ("
              << (rt_triangles_.size() * sizeof(RTTriangle) / (1024.0 * 1024.0)) << " MiB)" << std::endl;
}

Bvh::BVHBuildNode* Bvh::RecursiveBuild(const std::vector<Vertex>& vertices,
    const std::vector<std::uint32_t>& src_indices, std::vector<BVHPrimitiveInfo>& primitive_info, unsigned start,
    unsigned end, unsigned* total_nodes, std::vector<RTTriangle>& ordered_tris)
{
    BVHBuildNode* node = new BVHBuildNode;
    (*total_nodes)++;

    // Bounds' AABB
    Aabb bounds;
    for (unsigned i = start; i < end; ++i)
        bounds = Union(bounds, primitive_info[i].bounds);

    const unsigned n = end - start;

    auto push_triangle = [&](unsigned primId)
    {
        const uint32_t i0 = src_indices[3 * primId + 0];
        const uint32_t i1 = src_indices[3 * primId + 1];
        const uint32_t i2 = src_indices[3 * primId + 2];

        RTTriangle tri = {};
        tri.position1 = vertices[i0].position;
        tri.position2 = vertices[i1].position;
        tri.position3 = vertices[i2].position;
        tri.prim_id = primId;

        ordered_tris.push_back(tri);
    };

    if (n == 1)
    {
        const unsigned first = static_cast<unsigned>(ordered_tris.size());
        push_triangle(primitive_info[start].primitiveNumber);
        node->InitLeaf(first, 1, bounds);
        return node;
    }

    Aabb cBounds;
    for (unsigned i = start; i < end; ++i)
        cBounds = Union(cBounds, primitive_info[i].centroid);
    const unsigned dim = cBounds.MaximumExtent();

    if (cBounds.max[dim] == cBounds.min[dim])
    {
        const unsigned first = static_cast<unsigned>(ordered_tris.size());
        for (unsigned i = start; i < end; ++i)
            push_triangle(primitive_info[i].primitiveNumber);
        node->InitLeaf(first, n, bounds);
        return node;
    }

    unsigned mid = (start + end) / 2;
    if (n <= 2)
    {
        std::nth_element(&primitive_info[start],
            &primitive_info[mid],
            &primitive_info[end - 1] + 1,
            [dim](const BVHPrimitiveInfo& a, const BVHPrimitiveInfo& b) { return a.centroid[dim] < b.centroid[dim]; });
    }
    else
    {
        constexpr unsigned nBuckets = 12;
        BucketInfo buckets[nBuckets]{};

        for (unsigned i = start; i < end; ++i)
        {
            int b = int(nBuckets * cBounds.Offset(primitive_info[i].centroid)[dim]);
            if (b == int(nBuckets))
                b = int(nBuckets) - 1;
            buckets[b].count++;
            buckets[b].bounds = Union(buckets[b].bounds, primitive_info[i].bounds);
        }

        float cost[nBuckets - 1];
        for (unsigned i = 0; i < nBuckets - 1; ++i)
        {
            Aabb b0, b1;
            int c0 = 0, c1 = 0;
            for (unsigned j = 0; j <= i; ++j)
            {
                b0 = Union(b0, buckets[j].bounds);
                c0 += buckets[j].count;
            }
            for (unsigned j = i + 1; j < nBuckets; ++j)
            {
                b1 = Union(b1, buckets[j].bounds);
                c1 += buckets[j].count;
            }
            cost[i] = 1.0f + (c0 * b0.SurfaceArea() + c1 * b1.SurfaceArea()) / bounds.SurfaceArea();
        }

        float minCost = cost[0];
        unsigned minSplit = 0;
        for (unsigned i = 1; i < nBuckets - 1; ++i)
            if (cost[i] < minCost)
            {
                minCost = cost[i];
                minSplit = i;
            }

        const float leafCost = float(n);
        if (n > kMaxPrimitivesInNode || minCost < leafCost)
        {
            BVHPrimitiveInfo* pmid = std::partition(&primitive_info[start],
                &primitive_info[end - 1] + 1,
                [&](const BVHPrimitiveInfo& pi)
                {
                    int b = int(nBuckets * cBounds.Offset(pi.centroid)[dim]);
                    if (b == int(nBuckets))
                        b = int(nBuckets) - 1;
                    return unsigned(b) <= minSplit;
                });
            mid = unsigned(pmid - &primitive_info[0]);
        }
        else
        {
            const unsigned first = static_cast<unsigned>(ordered_tris.size());
            for (unsigned i = start; i < end; ++i)
                push_triangle(primitive_info[i].primitiveNumber);
            node->InitLeaf(first, n, bounds);
            return node;
        }
    }

    node->InitInterior(dim,
        RecursiveBuild(vertices, src_indices, primitive_info, start, mid, total_nodes, ordered_tris),
        RecursiveBuild(vertices, src_indices, primitive_info, mid, end, total_nodes, ordered_tris));
    return node;
}

unsigned int Bvh::FlattenBVHTree(BVHBuildNode* node, unsigned int* offset)
{
    LinearBVHNode* linearNode = &nodes_[*offset];
    linearNode->bmin = glm::vec4(node->bounds.min.x, node->bounds.min.y, node->bounds.min.z, 0.0f);
    linearNode->bmax = glm::vec4(node->bounds.max.x, node->bounds.max.y, node->bounds.max.z, 0.0f);
    unsigned int myOffset = (*offset)++;
    if (node->nPrimitives > 0)
    {
        assert(!node->children[0] && !node->children[1]);
        assert(node->nPrimitives < 65536);
        linearNode->offset = node->firstPrimOffset;
        linearNode->num_primitives_axis = node->nPrimitives << 16;
    }
    else
    {
        // Create interior flattened BVH node
        linearNode->num_primitives_axis = node->splitAxis;
        // linearNode->nPrimitives = 0;
        FlattenBVHTree(node->children[0], offset);
        linearNode->offset = FlattenBVHTree(node->children[1], offset);
    }

    return myOffset;
}
