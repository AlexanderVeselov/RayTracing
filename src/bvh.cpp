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

namespace
{
glm::vec3 TransformPosition(InstanceInfo const& instance, glm::vec3 const& position)
{
    return glm::vec3(instance.transform * glm::vec4(position, 1.0f));
}
}  // namespace

void Bvh::BuildCPU(std::vector<Vertex> const& vertices, std::vector<uint32_t> const& indices,
    std::vector<MeshInfo> const& meshes, std::vector<InstanceInfo> const& instances)
{
    std::cout << "Building BVH (VB/IB -> compact tri buffer) ..." << std::endl;

    nodes_.clear();
    rt_triangles_.clear();

    unsigned triangle_count = 0;
    for (InstanceInfo const& instance : instances)
    {
        if (instance.mesh_index >= meshes.size())
        {
            throw std::runtime_error("Bvh::BuildCPU: instance mesh index is out of range");
        }

        triangle_count += meshes[instance.mesh_index].triangle_count;
    }

    // 1. Collect primitives: AABB and centroids based on VB/IB
    std::vector<BVHPrimitiveInfo> prim_info(triangle_count);
    unsigned primitive_info_index = 0;

    for (unsigned instance_index = 0; instance_index < instances.size(); ++instance_index)
    {
        InstanceInfo const& instance = instances[instance_index];
        MeshInfo const& mesh = meshes[instance.mesh_index];

        for (uint32_t primitive_index = 0; primitive_index < mesh.triangle_count; ++primitive_index)
        {
            uint32_t index_offset = mesh.index_offset + primitive_index * 3;
            const uint32_t i0 = mesh.vertex_offset + indices[index_offset + 0];
            const uint32_t i1 = mesh.vertex_offset + indices[index_offset + 1];
            const uint32_t i2 = mesh.vertex_offset + indices[index_offset + 2];

            const glm::vec3 p0 = TransformPosition(instance, glm::vec3(vertices[i0].position));
            const glm::vec3 p1 = TransformPosition(instance, glm::vec3(vertices[i1].position));
            const glm::vec3 p2 = TransformPosition(instance, glm::vec3(vertices[i2].position));

            Aabb tri_aabb;
            tri_aabb = Union(tri_aabb, p0);
            tri_aabb = Union(tri_aabb, p1);
            tri_aabb = Union(tri_aabb, p2);
            const glm::vec3 c = (p0 + p1 + p2) * (1.0f / 3.0f);

            prim_info[primitive_info_index++] = {primitive_index, instance_index, tri_aabb, c};
        }
    }

    // 2. Recursively build
    unsigned total_nodes = 0;
    rt_triangles_.reserve(triangle_count);
    root_node_ = RecursiveBuild(vertices,
        indices,
        meshes,
        instances,
        prim_info,
        0,
        triangle_count,
        &total_nodes,
        rt_triangles_);

    // 3. Flatten
    nodes_.resize(total_nodes);
    unsigned off = 0;
    FlattenBVHTree(root_node_, &off);
    assert(off == total_nodes);

    std::cout << "BVH nodes: " << total_nodes << ", tris in buffer: " << rt_triangles_.size() << " ("
              << (rt_triangles_.size() * sizeof(RTTriangle) / (1024.0 * 1024.0)) << " MiB)" << std::endl;
}

Bvh::BVHBuildNode* Bvh::RecursiveBuild(std::vector<Vertex> const& vertices, std::vector<uint32_t> const& src_indices,
    std::vector<MeshInfo> const& meshes, std::vector<InstanceInfo> const& instances,
    std::vector<BVHPrimitiveInfo>& primitive_info, unsigned start, unsigned end, unsigned* total_nodes,
    std::vector<RTTriangle>& ordered_tris)
{
    BVHBuildNode* node = new BVHBuildNode;
    (*total_nodes)++;

    // Bounds' AABB
    Aabb bounds;
    for (unsigned i = start; i < end; ++i)
        bounds = Union(bounds, primitive_info[i].bounds);

    const unsigned n = end - start;

    auto push_triangle = [&](BVHPrimitiveInfo const& primitive)
    {
        InstanceInfo const& instance = instances[primitive.instanceIndex];
        MeshInfo const& mesh = meshes[instance.mesh_index];
        uint32_t index_offset = mesh.index_offset + primitive.primitiveNumber * 3;

        const uint32_t i0 = mesh.vertex_offset + src_indices[index_offset + 0];
        const uint32_t i1 = mesh.vertex_offset + src_indices[index_offset + 1];
        const uint32_t i2 = mesh.vertex_offset + src_indices[index_offset + 2];

        RTTriangle tri = {};
        tri.position1 = TransformPosition(instance, vertices[i0].position);
        tri.position2 = TransformPosition(instance, vertices[i1].position);
        tri.position3 = TransformPosition(instance, vertices[i2].position);
        tri.prim_id = primitive.primitiveNumber;
        tri.instance_id = primitive.instanceIndex;

        ordered_tris.push_back(tri);
    };

    if (n == 1)
    {
        const unsigned first = static_cast<unsigned>(ordered_tris.size());
        push_triangle(primitive_info[start]);
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
            push_triangle(primitive_info[i]);
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
                push_triangle(primitive_info[i]);
            node->InitLeaf(first, n, bounds);
            return node;
        }
    }

    node->InitInterior(dim,
        RecursiveBuild(vertices, src_indices, meshes, instances, primitive_info, start, mid, total_nodes, ordered_tris),
        RecursiveBuild(vertices, src_indices, meshes, instances, primitive_info, mid, end, total_nodes, ordered_tris));
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
