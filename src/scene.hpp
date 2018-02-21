#ifndef SCENE_HPP
#define SCENE_HPP

#include "mathlib.hpp"
#include "aabb.hpp"
#include "shared_structs.hpp"
#include <CL/cl.hpp>
#include <algorithm>
#include <vector>
#include <map>

class Scene
{
public:
    Scene(const char* filename);
    virtual void SetupBuffers() = 0;
    virtual void DrawDebug() = 0;
    const std::vector<Triangle>& GetTriangles() const;
    
private:
    void LoadTriangles(const char* filename);
    void LoadMtlFile(const char* filename);
    std::map<std::string, Material> m_Materials;

protected:
    std::vector<Triangle> m_Triangles;
    cl::Buffer m_TriangleBuffer;

};

class UniformGridScene : public Scene
{
public:
    UniformGridScene(const char* filename);
    virtual void SetupBuffers();
    virtual void DrawDebug();

private:
    void CreateGrid(unsigned int cellResolution);

    std::vector<unsigned int> m_Indices;
    std::vector<CellData> m_Cells;
    cl::Buffer m_IndexBuffer;
    cl::Buffer m_CellBuffer;

};

struct BVHBuildNode;
struct BVHPrimitiveInfo;

class BVHScene : public Scene
{
public:
    BVHScene(const char* filename, int maxPrimitivesInNode);
    virtual void SetupBuffers();
    virtual void DrawDebug();

private:
    BVHBuildNode *BVHScene::RecursiveBuild(
        std::vector<BVHPrimitiveInfo> &primitiveInfo,
        int start,
        int end, int *totalNodes,
        std::vector<Triangle> &orderedTriangles);

    int BVHScene::FlattenBVHTree(BVHBuildNode *node, int *offset);

private:
    std::vector<LinearBVHNode> m_Nodes;
    int m_MaxPrimitivesInNode;
    cl::Buffer m_NodeBuffer;
    BVHBuildNode* m_Root;

};


/*
class Sphere
{
public:
    Sphere(float3 Position, float Radius) :
        pos(Position),
        color(float3(Position.z, Position.y, Position.x).normalize()),
        r(Radius)
    {}

    // Getters...
    float3 GetPosition() const { return pos;    }
    float3 GetColor()    const { return color;    }
    float  GetRadius()   const { return r;        }

private:
    float3 pos;
    float3 color;
    float  r;
    // Padded to align on 4 floats due to technical reasons
    float  unused[3];

};
*/

#endif // SCENE_HPP
