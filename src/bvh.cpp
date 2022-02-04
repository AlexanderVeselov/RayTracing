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

#include "bvh.hpp"
#include "gpu_wrappers/cl_context.hpp"
#include "Utils/cl_exception.hpp"

namespace
{
    constexpr auto kMaxPrimitivesInNode = 4u;
}

Bvh::Bvh()
{
}

void Bvh::BuildCPU(std::vector<Triangle> & triangles)
{
    std::cout << "Building Bounding Volume Hierarchy for scene" << std::endl;

    std::uint32_t max_prims_in_node = 4u;

    //double startTime = m_Render.GetCurtime();
    std::vector<BVHPrimitiveInfo> primitiveInfo(triangles.size());
    for (unsigned int i = 0; i < triangles.size(); ++i)
    {
        primitiveInfo[i] = { i, triangles[i].GetBounds() };
    }

    unsigned int totalNodes = 0;
    std::vector<Triangle> orderedTriangles;
    root_node_ = RecursiveBuild(triangles, primitiveInfo, 0, triangles.size(), &totalNodes, orderedTriangles);
    triangles.swap(orderedTriangles);

    //primitiveInfo.resize(0);
    std::cout << "BVH created with " << totalNodes << " nodes for " << triangles.size()
        << " triangles (" << float(totalNodes * sizeof(BVHBuildNode)) / (1024.0f * 1024.0f) << " MB, "
        //<< m_Render.GetCurtime() - startTime << "s elapsed)"
        << std::endl;

    // Compute representation of depth-first traversal of BVH tree
    nodes_.resize(totalNodes);
    unsigned int offset = 0;
    FlattenBVHTree(root_node_, &offset);
    assert(totalNodes == offset);
}

Bvh::BVHBuildNode* Bvh::RecursiveBuild(
    std::vector<Triangle> const& triangles,
    std::vector<BVHPrimitiveInfo>& primitiveInfo,
    unsigned int start,
    unsigned int end, unsigned int* totalNodes,
    std::vector<Triangle>& orderedTriangles)
{
    assert(start <= end);

    //// TODO: Add memory pool
    BVHBuildNode* node = new BVHBuildNode;
    (*totalNodes)++;

    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    for (unsigned int i = start; i < end; ++i)
    {
        bounds = Union(bounds, primitiveInfo[i].bounds);
    }

    unsigned int nPrimitives = end - start;
    if (nPrimitives == 1)
    {
        // Create leaf
        int firstPrimOffset = orderedTriangles.size();
        for (unsigned int i = start; i < end; ++i)
        {
            int primNum = primitiveInfo[i].primitiveNumber;
            orderedTriangles.push_back(triangles[primNum]);
        }
        node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
        return node;
    }
    else
    {
        // Compute bound of primitive centroids, choose split dimension
        Bounds3 centroidBounds;
        for (unsigned int i = start; i < end; ++i)
        {
            centroidBounds = Union(centroidBounds, primitiveInfo[i].centroid);
        }
        unsigned int dim = centroidBounds.MaximumExtent();

        // Partition primitives into two sets and build children
        unsigned int mid = (start + end) / 2;
        if (centroidBounds.max[dim] == centroidBounds.min[dim])
        {
            // Create leaf
            unsigned int firstPrimOffset = orderedTriangles.size();
            for (unsigned int i = start; i < end; ++i)
            {
                unsigned int primNum = primitiveInfo[i].primitiveNumber;
                orderedTriangles.push_back(triangles[primNum]);
            }
            node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
            return node;
        }
        else
        {
            if (nPrimitives <= 2)
            {
                // Partition primitives into equally-sized subsets
                mid = (start + end) / 2;
                std::nth_element(&primitiveInfo[start], &primitiveInfo[mid], &primitiveInfo[end - 1] + 1,
                    [dim](const BVHPrimitiveInfo& a, const BVHPrimitiveInfo& b)
                    {
                        return a.centroid[dim] < b.centroid[dim];
                    });
            }
            else
            {
                // Partition primitives using approximate SAH
                const unsigned int nBuckets = 12;
                BucketInfo buckets[nBuckets];

                // Initialize _BucketInfo_ for SAH partition buckets
                for (unsigned int i = start; i < end; ++i)
                {
                    int b = nBuckets * centroidBounds.Offset(primitiveInfo[i].centroid)[dim];
                    if (b == nBuckets) b = nBuckets - 1;
                    assert(b >= 0 && b < nBuckets);
                    buckets[b].count++;
                    buckets[b].bounds = Union(buckets[b].bounds, primitiveInfo[i].bounds);
                }

                // Compute costs for splitting after each bucket
                float cost[nBuckets - 1];
                for (unsigned int i = 0; i < nBuckets - 1; ++i)
                {
                    Bounds3 b0, b1;
                    int count0 = 0, count1 = 0;
                    for (unsigned int j = 0; j <= i; ++j)
                    {
                        b0 = Union(b0, buckets[j].bounds);
                        count0 += buckets[j].count;
                    }
                    for (unsigned int j = i + 1; j < nBuckets; ++j)
                    {
                        b1 = Union(b1, buckets[j].bounds);
                        count1 += buckets[j].count;
                    }
                    cost[i] = 1.0f + (count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea()) / bounds.SurfaceArea();
                }

                // Find bucket to split at that minimizes SAH metric
                float minCost = cost[0];
                unsigned int minCostSplitBucket = 0;
                for (unsigned int i = 1; i < nBuckets - 1; ++i)
                {
                    if (cost[i] < minCost)
                    {
                        minCost = cost[i];
                        minCostSplitBucket = i;
                    }
                }

                // Either create leaf or split primitives at selected SAH bucket
                float leafCost = float(nPrimitives);
                if (nPrimitives > kMaxPrimitivesInNode || minCost < leafCost)
                {
                    BVHPrimitiveInfo* pmid = std::partition(&primitiveInfo[start], &primitiveInfo[end - 1] + 1,
                        [=](const BVHPrimitiveInfo& pi)
                        {
                            int b = nBuckets * centroidBounds.Offset(pi.centroid)[dim];
                            if (b == nBuckets) b = nBuckets - 1;
                            assert(b >= 0 && b < nBuckets);
                            return b <= minCostSplitBucket;
                        });
                    mid = pmid - &primitiveInfo[0];
                }
                else
                {
                    // Create leaf
                    unsigned int firstPrimOffset = orderedTriangles.size();
                    for (unsigned int i = start; i < end; ++i)
                    {
                        unsigned int primNum = primitiveInfo[i].primitiveNumber;
                        orderedTriangles.push_back(triangles[primNum]);
                    }
                    node->InitLeaf(firstPrimOffset, nPrimitives, bounds);

                    return node;
                }
            }

            node->InitInterior(dim,
                RecursiveBuild(triangles, primitiveInfo, start, mid,
                    totalNodes, orderedTriangles),
                RecursiveBuild(triangles, primitiveInfo, mid, end,
                    totalNodes, orderedTriangles));
        }
    }

    return node;
}

unsigned int Bvh::FlattenBVHTree(BVHBuildNode* node, unsigned int* offset)
{
    LinearBVHNode* linearNode = &nodes_[*offset];
    linearNode->bounds = node->bounds;
    unsigned int myOffset = (*offset)++;
    if (node->nPrimitives > 0)
    {
        assert(!node->children[0] && !node->children[1]);
        assert(node->nPrimitives < 65536);
        linearNode->offset = node->firstPrimOffset;
        linearNode->nPrimitives = node->nPrimitives;
    }
    else
    {
        // Create interior flattened BVH node
        linearNode->axis = node->splitAxis;
        linearNode->nPrimitives = 0;
        FlattenBVHTree(node->children[0], offset);
        linearNode->offset = FlattenBVHTree(node->children[1], offset);
    }

    return myOffset;
}
