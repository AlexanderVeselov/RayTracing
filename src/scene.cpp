#include "scene.hpp"
#include "mathlib.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include <unordered_map>
#include <cctype>

class FileReader
{
public:
    FileReader(const char* filename) : m_InputFile(filename), m_VectorValue(0.0f)
    {
        if (!m_InputFile)
        {
            std::cerr << "Failed to load " << filename << std::endl;
        }
    }

    std::string GetStringValue() const
    {
        return m_StringValue;
    }

    float3 GetVectorValue() const
    {
        return m_VectorValue;
    }

    float3 GetFloatValue() const
    {
        return m_VectorValue;
    }

protected:
    float ReadFloatValue()
    {
        SkipSpaces();
        float value = 0.0f;
        bool minus = m_CurrentLine[m_CurrentChar] == '-';
        if (minus) ++m_CurrentChar;
        while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
        {
            value = value * 10.0f + (float)((int)m_CurrentLine[m_CurrentChar++] - 48);
        }
        if (m_CurrentLine[m_CurrentChar++] == '.')
        {
            size_t frac = 1;
            while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
            {
                value += (float)((int)m_CurrentLine[m_CurrentChar++] - 48) / (std::powf(10.0f, frac++));
            }
        }

        m_FloatValue = minus ? -value : value;
        return m_FloatValue;
        
    }

    int ReadIntValue()
    {
        SkipSpaces();
        int value = 0;
        bool minus = m_CurrentLine[m_CurrentChar] == '-';
        if (minus) ++m_CurrentChar;

        while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
        {
            value = value * 10 + ((int)m_CurrentLine[m_CurrentChar++] - 48);
        }
        return minus ? -value : value;
    }

    void ReadVectorValue()
    {
        m_VectorValue.x = ReadFloatValue();
        m_VectorValue.y = ReadFloatValue();
        m_VectorValue.z = ReadFloatValue();

    }

    void ReadStringValue()
    {
        SkipSpaces();
        m_StringValue.clear();

        if (m_CurrentChar < m_CurrentLine.size() && IsIdentifierStart(m_CurrentLine[m_CurrentChar]))
        {
            m_StringValue += m_CurrentLine[m_CurrentChar++];
            while (m_CurrentChar < m_CurrentLine.size() && IsIdentifierBody(m_CurrentLine[m_CurrentChar]))
            {
                m_StringValue += m_CurrentLine[m_CurrentChar++];
            }
        }

    }

    bool ReadLine()
    {
        do
        {
            if (!std::getline(m_InputFile, m_CurrentLine))
            {
                return false;
            }
        } while (m_CurrentLine.size() == 0);

        m_CurrentChar = 0;
        return true;
    }

    bool IsIdentifierStart(char symbol)
    {
        return symbol >= 'a' && symbol <= 'z' ||
            symbol >= 'A' && symbol <= 'Z' || symbol == '_';
    }

    bool IsIdentifierBody(char symbol)
    {
        return IsIdentifierStart(symbol) || symbol >= '0' && symbol <= '9';
    }

    void SkipSpaces()
    {
        while (std::isspace(m_CurrentLine[m_CurrentChar])) { m_CurrentChar++; }
    }

    void SkipSymbol(char symbol)
    {
        SkipSpaces();
        while (m_CurrentLine[m_CurrentChar] == symbol) { m_CurrentChar++; }
    }

private:
    std::ifstream m_InputFile;
    std::string m_CurrentLine;
    std::string m_StringValue;
    float3 m_VectorValue;
    float m_FloatValue;
    size_t m_CurrentChar;

};

class MtlReader : public FileReader
{
public:
    enum MtlToken_t
    {
        MTL_MTLNAME,
        MTL_DIFFUSE,
        MTL_SPECULAR,
        MTL_EMISSION,
        MTL_INVALID,
        MTL_EOF
    };

    MtlReader(const char* filename) : FileReader(filename)
    {
        std::cout << "Loading material library " << filename << std::endl;
    }

    MtlToken_t NextToken()
    {
        if (!ReadLine())
        {
            return MTL_EOF;
        }
        
        ReadStringValue();
        if (GetStringValue() == "newmtl")
        {
            ReadStringValue();
            return MTL_MTLNAME;
        }
        else if (GetStringValue() == "Kd")
        {
            ReadVectorValue();
            return MTL_DIFFUSE;
        }
        else if (GetStringValue() == "Ke")
        {
            ReadVectorValue();
            return MTL_EMISSION;
        }
        else if (GetStringValue() == "Ns")
        {
            ReadFloatValue();
            return MTL_SPECULAR;
        }
        else
        {
            return MTL_INVALID;
        }

    }
    
};

class ObjReader : public FileReader
{
public:
    enum ObjToken_t
    {
        OBJ_MTLLIB,
        OBJ_USEMTL,
        OBJ_POSITION,
        OBJ_TEXCOORD,
        OBJ_NORMAL,
        OBJ_FACE,
        OBJ_INVALID,
        OBJ_EOF
    };

    ObjReader(const char* filename) : FileReader(filename)
    {
        std::cout << "Loading object file " << filename << std::endl;
        
    }

    ObjToken_t NextToken()
    {
        if (!ReadLine())
        {
            return OBJ_EOF;
        }

        ReadStringValue();
        if (GetStringValue() == "mtllib")
        {
            ReadStringValue();
            return OBJ_MTLLIB;
        }
        else if (GetStringValue() == "usemtl")
        {
            ReadStringValue();
            return OBJ_USEMTL;
        }
        else if (GetStringValue() == "v")
        {
            ReadVectorValue();
            return OBJ_POSITION;
        }
        else if (GetStringValue() == "vn")
        {
            ReadVectorValue();
            return OBJ_NORMAL;
        }
        else if (GetStringValue() == "vt")
        {
            ReadVectorValue();
            return OBJ_TEXCOORD;
        }
        else if (GetStringValue() == "f")
        {
            for (size_t i = 0; i < 3; ++i)
            {
                m_VertexIndices[i] = ReadIntValue() - 1;
                SkipSymbol('/');
                m_TexcoordIndices[i] = ReadIntValue() - 1;
                SkipSymbol('/');
                m_NormalIndices[i] = ReadIntValue() - 1;

            }
            return OBJ_FACE;
        }
        else
        {
            return OBJ_INVALID;
        }
    }
    
    void GetFaceIndices(unsigned int** iv, unsigned int** it, unsigned int** in)
    {
        *iv = m_VertexIndices;
        *it = m_TexcoordIndices;
        *in = m_NormalIndices;
    }

private:
    // Face indices
    unsigned int m_VertexIndices[3];
    unsigned int m_TexcoordIndices[3];
    unsigned int m_NormalIndices[3];

};

Scene::Scene(const char* filename, size_t cell_resolution) : m_Filename(filename), m_CellResolution(cell_resolution), m_SceneBounds(0.0f, 0.0f)
{
    LoadTriangles();
    CreateGrid();
}

void Scene::CreateGrid()
{
    //typedef std::vector<Triangle>::const_iterator iter_t;
    std::vector<cl_uint> lower_indices;
    std::vector<CellData> lower_cells;
    clock_t t = clock();
    int resolution = 2;
    std::cout << "Creating uniform grid" << std::endl;
    std::cout << "Scene bounding box min: " << m_SceneBounds.min << " max: " << m_SceneBounds.max << std::endl;
    std::cout << "Resolution: ";
    while (resolution <= m_CellResolution)
    {
        std::cout << resolution << "... ";
        float InvResolution = 1.0f / static_cast<float>(resolution);
        float3 dv = (m_SceneBounds.max - m_SceneBounds.min) / static_cast<float>(resolution);
        CellData current_cell = { 0, 0 };

        lower_indices = indices;
        lower_cells = cells;
        indices.clear();
        cells.clear();

        int totalCells = resolution * resolution * resolution;
        for (size_t z = 0; z < resolution; ++z)
            for (size_t y = 0; y < resolution; ++y)
                for (size_t x = 0; x < resolution; ++x)
                {
                    Bounds3 cellBound(m_SceneBounds.min + float3(x*dv.x, y*dv.y, z*dv.z), m_SceneBounds.min + float3((x + 1)*dv.x, (y + 1)*dv.y, (z + 1)*dv.z));

                    current_cell.start_index = indices.size();

                    if (resolution == 2)
                    {
                        for (size_t i = 0; i < triangles.size(); ++i)
                        {
                            if (cellBound.Intersects(triangles[i]))
                            {
                                indices.push_back(i);
                            }
                        }
                    }
                    else
                    {
                        CellData lower_cell = lower_cells[x / 2 + (y / 2) * (resolution / 2) + (z / 2) * (resolution / 2) * (resolution / 2)];
                        for (int i = lower_cell.start_index; i < lower_cell.start_index + lower_cell.count; ++i)
                        {
                            if (cellBound.Intersects(triangles[lower_indices[i]]))
                            {
                                indices.push_back(lower_indices[i]);
                            }
                        }
                    }

                    current_cell.count = indices.size() - current_cell.start_index;
                    cells.push_back(current_cell);

                }
        resolution *= 2;

    }

    std::cout << "Done (" << (clock() - t) << " ms elapsed)" << std::endl;

}

void Scene::LoadMtlFile(const char* filename)
{
    MtlReader mtlReader(filename);

    MtlReader::MtlToken_t token;
    Material currentMaterial;
    std::string materialName;
    while ((token = mtlReader.NextToken()) != MtlReader::MTL_EOF)
    {
        switch (token)
        {
        case MtlReader::MTL_MTLNAME:
            materialName = mtlReader.GetStringValue();
            break;
        case MtlReader::MTL_DIFFUSE:
            currentMaterial.diffuse = mtlReader.GetVectorValue();
            break;
        case MtlReader::MTL_SPECULAR:
            currentMaterial.specular = mtlReader.GetVectorValue();
            break;
        case MtlReader::MTL_EMISSION:
            currentMaterial.emission = mtlReader.GetVectorValue();
            materials[materialName] = currentMaterial;
            break;
        }
    }
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

void Scene::LoadTriangles()
{
    ObjReader objReader(m_Filename);

    std::vector<float3> positions;
    std::vector<float3> normals;
    std::vector<float2> texcoords;
    //unsigned int newIndex = 0;
    //unsigned int existingIndex = 0;

    Material currentMaterial;
    //std::string materialName;
    ObjReader::ObjToken_t token;

    std::unordered_map<std::string, unsigned int> indexDictionary;
    std::unordered_map<std::string, unsigned int>::iterator iter;

    while ((token = objReader.NextToken()) != ObjReader::OBJ_EOF)
    {
        switch (token)
        {
        case ObjReader::OBJ_MTLLIB:
            LoadMtlFile(("meshes/" + objReader.GetStringValue() + ".mtl").c_str());
            break;
        case ObjReader::OBJ_POSITION:
            positions.push_back(objReader.GetVectorValue());
            // Union bounds
            break;
        case ObjReader::OBJ_TEXCOORD:
            texcoords.push_back(float2(objReader.GetVectorValue().x, 1.0f - objReader.GetVectorValue().y));
            break;
        case ObjReader::OBJ_NORMAL:
            normals.push_back(objReader.GetVectorValue());
            break;
        case ObjReader::OBJ_USEMTL:
            currentMaterial = materials[objReader.GetStringValue()];
            break;
        case ObjReader::OBJ_FACE:
            unsigned int *iv, *it, *in;
            objReader.GetFaceIndices(&iv, &it, &in);
            triangles.push_back(Triangle(
                Vertex(positions[iv[0]], texcoords[it[0]], normals[in[0]]),
                Vertex(positions[iv[1]], texcoords[it[1]], normals[in[1]]),
                Vertex(positions[iv[2]], texcoords[it[2]], normals[in[2]])
                ));
            break;
        }
    }

    for (int i = 0; i < 3; ++i)
    {
        m_SceneBounds.max[i] = 64;
        m_SceneBounds.min[i] = 0;
    }
    
    std::cout << "Load succesful (" << triangles.size() << " triangles)" << std::endl;

}
